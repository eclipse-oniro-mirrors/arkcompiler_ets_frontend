/**
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import * as os from 'os';
import * as path from 'path';
import * as fs from 'fs';
import * as child_process from 'child_process';
import cloneDeep from 'lodash.clonedeep';

import {
    ARKTSCONFIG_JSON_FILE,
    DEP_ANALYZER_DIR,
    DEP_ANALYZER_INPUT_FILE,
    DEP_ANALYZER_OUTPUT_FILE,
    FILE_HASH_CACHE,
    CLUSTER_FILES_THRESHOLD,
    ENABLE_CLUSTERS,
    ENABLE_DECL_FILE_CACHE,
    DECL_ETS_SUFFIX,
    ABC_SUFFIX
} from '../pre_define';

import {
    changeFileExtension,
    shouldBeUpdated,
    updateFileHash,
    ensureDirExists,
    ensurePathExists,
    isMac,
    computeHash
} from '../util/utils';

import {
    BuildConfig,
    ModuleInfo,
    CompileJobInfo,
    CompileJobType,
    JobContentType,
    JobInfo,
    FileInfo,
    OHOS_MODULE_TYPE,
    isHarOrHsp,
    FileChangeStatus,
    getDefaultFileChangeStatus
} from '../types';

import { Logger, LogDataFactory } from '../logger';
import { StatisticsRecorder, RecordEvent, BS_PERF_DIR } from '../util/statsRecorder';
import { ErrorCode, DriverError } from '../util/error';
import { ArkTSConfigGenerator, ArkTSConfig } from '../build/generate_arktsconfig';
import { Graph, GraphNode } from '../util/graph';
import { dotGraphDump } from '../util/dotGraphDump';

export interface DependencyFileMap {
    dependants: Record<string, string[]>;
    dependencies: Record<string, string[]>;
    outputMatching: Record<string, string>;
}

export function MakeEmptyDepFileMap(): DependencyFileMap {
    return {
        dependants: {},
        dependencies: {},
        outputMatching: {}
    };
}

enum DepAnalyzerEvent {
    GEN_DEPENDENCY_MAP = 'Gen Dep Map',
    CREATE_GRAPH = 'Create graph',
    COLLAPSE_CYCLES = 'Collapse Cycles',
    FIRST_FILTER_GRAPH = 'First Filter Graph',
    CLUSTER_GRAPH = 'Create Cluster',
    SECOND_FILTER_GRAPH = 'Second Filter Graph',
    SAVE_HASH = 'Save Hashes'
}

function formEvent(event: DepAnalyzerEvent): string {
    return event;
}

export interface DepGraphContext {
    entryFiles: Set<string>;
    fileToModule: Map<string, ModuleInfo>;
    dependencyMap: DependencyFileMap;
}

/**
 * Dependency Analyzer base class
 */
export abstract class DepAnalyzer {
    private readonly logger: Logger;
    private readonly binPath: string;
    protected readonly outputDir: string;
    private readonly cacheDir: string;
    private readonly hashCacheFile: string;
    protected readonly statsRecorder: StatisticsRecorder;
    private readonly dumpGraph: boolean = false;
    private readonly clusteredBuild: boolean = false;
    protected readonly mainModuleType: OHOS_MODULE_TYPE;
    private readonly generator: ArkTSConfigGenerator;
    protected readonly declgenV2OutDir: string;
    private entryFiles: Set<string>;
    protected filesHashCache: Record<string, string>;
    protected deletedFiles: string[];
    private filesChangeStatusCache: Record<string, FileChangeStatus>;

    constructor(
        buildConfig: BuildConfig,
        generator: ArkTSConfigGenerator,
        clusteredBuild: boolean = ENABLE_CLUSTERS
    ) {
        this.logger = Logger.getInstance();
        this.generator = generator;
        this.entryFiles = new Set<string>(buildConfig.compileFiles);
        this.cacheDir = buildConfig.cachePath;
        this.outputDir = path.join(buildConfig.cachePath, DEP_ANALYZER_DIR);
        ensureDirExists(this.outputDir);
        this.binPath = buildConfig.dependencyAnalyzerPath!;

        this.hashCacheFile = path.resolve(buildConfig.cachePath, DEP_ANALYZER_DIR, FILE_HASH_CACHE);
        const loadResult = this.loadHashCache();
        this.filesHashCache = loadResult.hashCache;
        this.deletedFiles = loadResult.deletedFiles;
        this.filesChangeStatusCache = {};

        this.statsRecorder = new StatisticsRecorder(
            path.resolve(this.cacheDir, BS_PERF_DIR),
            'Dependency analyzer'
        );

        this.clusteredBuild = clusteredBuild;
        this.dumpGraph = buildConfig.dumpDependencyGraph ?? false;
        this.mainModuleType = buildConfig.moduleType;
        this.declgenV2OutDir = buildConfig.declgenV2OutPath;
    }

    private loadHashCache(): {
        hashCache: Record<string, string>;
        deletedFiles: string[];
    } {
        try {
            if (!fs.existsSync(this.hashCacheFile)) {
                this.logger.printDebug(`no hash cache file: ${this.hashCacheFile}`);
                return { hashCache: {}, deletedFiles: [] };
            }

            const hashCache: Record<string, string> = {};
            const deletedFiles: string[] = [];
            const cacheContent: string = fs.readFileSync(this.hashCacheFile, 'utf-8');
            this.logger.printDebug(`cacheContent: ${cacheContent}`);
            const cacheData: Record<string, string> = JSON.parse(cacheContent);

            for (const [file, hash] of Object.entries(cacheData)) {
                if (this.entryFiles.has(file)) {
                    hashCache[file] = hash;
                } else {
                    deletedFiles.push(file);
                }
            }

            return { hashCache, deletedFiles };
        } catch (error) {
            if (error instanceof Error) {
                throw new DriverError(
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_LOAD_HASH_CACHE_FAIL,
                        'Failed to load hash cache.',
                        error.message
                    )
                );
            }
            throw error;
        }
    }

    private saveHashCache(): void {
        ensurePathExists(this.hashCacheFile);
        fs.writeFileSync(this.hashCacheFile, JSON.stringify(this.filesHashCache, null, 2));
    }

    protected generateMergedArktsConfig(modules: ModuleInfo[], outputPath: string): void {
        const mainModule = modules.find(module => module.isMainModule)!;
        // NOTE: create new temporary arktsconfig for dependency analyzer
        const resArkTSConfig: ArkTSConfig = cloneDeep(this.getArktsConfigByPackageName(mainModule.packageName)!);

        modules.forEach(module => {
            if (module.isMainModule) {
                return;
            }
            resArkTSConfig.mergeArktsConfig(
                this.generator.getArktsConfigByPackageName(module.packageName)!
            );
        });

        fs.writeFileSync(outputPath, JSON.stringify(resArkTSConfig.object, null, 2));
    }

    private getArktsConfigByPackageName(name: string): ArkTSConfig | undefined {
        return this.generator.getArktsConfigByPackageName(name);
    }

    protected formExecCmd(input: string, output: string, config: string): string {
        const cmd = [
            `"${path.resolve(this.binPath)}"`,
            `@"${input}"`,
            `--arktsconfig="${config}"`,
            `--output="${output}"`
        ];
        let res = cmd.join(' ');

        if (isMac()) {
            const loadLibrary = `DYLD_LIBRARY_PATH="${process.env.DYLD_LIBRARY_PATH}"`;
            res = `${loadLibrary} ${res}`;
        }

        return res;
    }

    protected filterDependencyMap(
        dependencyMap: DependencyFileMap,
        entryFiles: Set<string>
    ): DependencyFileMap {
        Object.keys(dependencyMap.dependants).forEach(file => {
            if (!(file in dependencyMap.dependencies)) {
                dependencyMap.dependencies[file] = [];
            }
        });

        const resDependencyMap: DependencyFileMap = MakeEmptyDepFileMap();

        // Filter files by entryFiles
        Object.entries(dependencyMap.dependencies).forEach(([file, dependencies]) => {
            if (!entryFiles.has(file)) {
                return;
            }
            resDependencyMap.dependencies[file] = [...dependencies].filter(dep => entryFiles.has(dep));
        });

        Object.entries(dependencyMap.dependants).forEach(([file, dependants]) => {
            if (!entryFiles.has(file)) {
                return;
            }
            resDependencyMap.dependants[file] = [...dependants].filter(dep => entryFiles.has(dep));
        });

        resDependencyMap.outputMatching = dependencyMap.outputMatching;
        this.logger.printDebug(`filtered dependency map: ${JSON.stringify(resDependencyMap, null, 1)}`);
        return resDependencyMap;
    }

    protected get mergedArktsConfigPath(): string {
        return path.join(this.outputDir, ARKTSCONFIG_JSON_FILE);
    }

    protected generateDependencyMap(
        entryFiles: Set<string>,
        modules: ModuleInfo[],
        fileToModule: Map<string, ModuleInfo>
    ): DependencyFileMap {
        this.statsRecorder.record(formEvent(DepAnalyzerEvent.GEN_DEPENDENCY_MAP));
        const inputFile = path.join(this.outputDir, DEP_ANALYZER_INPUT_FILE);
        const outputFile = path.join(this.outputDir, DEP_ANALYZER_OUTPUT_FILE);
        const arktsConfigPath = this.mergedArktsConfigPath;

        const depAnalyzerInputFileContent = Array.from(entryFiles).join(os.EOL);
        fs.writeFileSync(inputFile, depAnalyzerInputFileContent);
        this.generateMergedArktsConfig(modules, arktsConfigPath);

        const execCmd = this.formExecCmd(inputFile, outputFile, arktsConfigPath);
        this.logger.printDebug(`Dependency analyzer cmd ${execCmd}`);

        try {
            child_process.execSync(execCmd, {
                stdio: 'pipe',
                encoding: 'utf-8'
            });
        } catch (error) {
            if (error instanceof Error) {
                const execError = error as child_process.ExecException;
                let fullErrorMessage = execError.message;
                if (execError.stderr) {
                    fullErrorMessage += `\nStdErr: ${execError.stderr}`;
                }
                if (execError.stdout) {
                    fullErrorMessage += `\nStdOutput: ${execError.stdout}`;
                }
                throw new DriverError(
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_DEPENDENCY_ANALYZE_FAIL,
                        'Failed to analyze files dependency.',
                        fullErrorMessage
                    )
                );
            }
            throw error;
        }

        const fullDependencyMap: DependencyFileMap = JSON.parse(fs.readFileSync(outputFile, 'utf-8'));
        this.logger.printDebug(`full dependency map: ${JSON.stringify(fullDependencyMap, null, 1)}`);
        return this.filterDependencyMap(fullDependencyMap, entryFiles);
    }

    protected setFileHashChanged(filePath: string, value: boolean): void {
        const status = this.filesChangeStatusCache[filePath];
        status.hashChanged.isSet = true;
        status.hashChanged.value = value;
    }

    protected setAbcOutdatedChanged(filePath: string, value: boolean): void {
        const status = this.filesChangeStatusCache[filePath];
        status.abcOutdated.isSet = true;
        status.abcOutdated.value = value;
    }

    protected setDeclOutdatedChanged(filePath: string, value: boolean): void {
        const status = this.filesChangeStatusCache[filePath];
        status.declOutdated.isSet = true;
        status.declOutdated.value = value;
    }

    protected getOrInitFileChangeStatus(filePath: string): FileChangeStatus {
        if (!this.filesChangeStatusCache[filePath]) {
            this.filesChangeStatusCache[filePath] = getDefaultFileChangeStatus();
        }
        return this.filesChangeStatusCache[filePath];
    }

    private verifyAndDumpGraph(graph: Graph<CompileJobInfo>, output: string): void {
        graph.verify();
        if (this.dumpGraph) {
            fs.writeFileSync(path.resolve(this.cacheDir, output), dotGraphDump(graph), 'utf-8');
        }
    }

    private fillNodePredecessors(node: GraphNode<CompileJobInfo>, sourceFile: string, depMap: DependencyFileMap) : void {
        const dependencies = depMap.dependencies[sourceFile] ?? [];
        for (const dependency of dependencies) {
            if (dependency !== sourceFile) {
                node.predecessors.add(computeHash(dependency));
            }
        }
    }

    private fillNodeDescendants(node: GraphNode<CompileJobInfo>, sourceFile: string, depMap: DependencyFileMap): void {
        const dependants = depMap.dependants[sourceFile] ?? [];
        for (const dependant of dependants) {
            if (dependant !== sourceFile) {
                node.descendants.add(computeHash(dependant));
            }
        }
    }

    private createDependencyGraph(
        entryFiles: Set<string>,
        fileToModule: Map<string, ModuleInfo>,
        dependencyMap: DependencyFileMap
    ): Graph<CompileJobInfo> {
        const dependencyGraphNodes: GraphNode<CompileJobInfo>[] = [];
        /*
         * Although we will set jobType in filterGraph again, but when there is a cycle in the dependency graph
         * we should recompile abc for hap
         * recompile abc and regenerate decl for har and hsp
         */
        let jobType = CompileJobType.ABC;
        if (isHarOrHsp(this.mainModuleType) && ENABLE_DECL_FILE_CACHE) {
            jobType |= CompileJobType.DECL;
        }

        for (const file of entryFiles) {
            const module: ModuleInfo = fileToModule.get(file)!;
            const node = new GraphNode<CompileJobInfo>(computeHash(file), {
                contentType: JobContentType.FILE,
                content: {
                    input: file,
                    output: dependencyMap.outputMatching[file] ?? changeFileExtension(file, ABC_SUFFIX)
                },
                arktsConfig: module.arktsConfigFile,
                moduleName: module.packageName,
                moduleRoot: module.moduleRootPath,
                declgenConfig: {
                    output: module.declgenV2OutPath!
                },
                jobType
            });

            this.fillNodePredecessors(node, file, dependencyMap);
            this.fillNodeDescendants(node, file, dependencyMap);

            dependencyGraphNodes.push(node);
        }

        return Graph.createGraphFromNodes(dependencyGraphNodes);
    }

    private checkClusterFilesChanged(
        files: FileInfo[],
        fileToModule: Map<string, ModuleInfo>
    ): CompileJobType {
        let jobType = CompileJobType.NONE;
        for (const fi of files) {
            let hashChanged = false;
            const fileChangeStatus = this.getOrInitFileChangeStatus(fi.input);

            // 1. file hash change check
            if (fileChangeStatus.hashChanged.isSet) {
                hashChanged = fileChangeStatus.hashChanged.value;
            } else {
                hashChanged = updateFileHash(fi.input, this.filesHashCache);
                this.setFileHashChanged(fi.input, hashChanged);
            }

            // 2. abc outdated check
            let abcOutdated = false;
            if (fileChangeStatus.abcOutdated.isSet) {
                abcOutdated = fileChangeStatus.abcOutdated.value;
            } else {
                abcOutdated = shouldBeUpdated(fi.input, fi.output);
                this.setAbcOutdatedChanged(fi.input, abcOutdated);
            }

            if (hashChanged || abcOutdated) {
                jobType |= CompileJobType.ABC;
            }

            // 3. decl outdated check for HAR/HSP
            if (ENABLE_DECL_FILE_CACHE && isHarOrHsp(this.mainModuleType)) {
                const module = fileToModule.get(fi.input);
                const relative = changeFileExtension(
                    path.relative(module?.moduleRootPath!, fi.input),
                    DECL_ETS_SUFFIX
                );
                const declEtsOutputPath = path.resolve(this.declgenV2OutDir, relative);
                let declOutdated = false;

                if (fileChangeStatus.declOutdated.isSet) {
                    declOutdated = fileChangeStatus.declOutdated.value;
                } else {
                    declOutdated = shouldBeUpdated(fi.input, declEtsOutputPath);
                    this.setDeclOutdatedChanged(fi.input, declOutdated);
                }

                if (hashChanged || declOutdated) {
                    jobType |= CompileJobType.DECL;
                }
            }
        }
        return jobType;
    }

    private updateNodeHashes(node: GraphNode<CompileJobInfo>): void {
        const files = node.data.contentType === JobContentType.FILE
            ? [node.data.content as FileInfo]
            : node.data.content as FileInfo[];

        for (const fi of files) {
            updateFileHash(fi.input, this.filesHashCache);
        }
    }

    private filterGraph(graph: Graph<CompileJobInfo>, fileToModule: Map<string, ModuleInfo>): void {
        for (const nodeId of Graph.topologicalSort(graph)) {
            const node = graph.getNodeById(nodeId);
            if (node.predecessors.size !== 0) {
                // Still has dependencies, skip remove, only refresh hash
                this.updateNodeHashes(node);
                continue;
            }

            const files = node.data.contentType === JobContentType.FILE
                ? [node.data.content as FileInfo]
                : node.data.content as FileInfo[];

            const jobType = this.checkClusterFilesChanged(files, fileToModule);
            if (jobType === CompileJobType.NONE) {
                this.logger.printDebug(
                    `Skipping ${node.data.contentType === JobContentType.FILE ? 'file' : 'cluster'} compilation: [${files.map(f => f.input).join(', ')}]`
                );
                graph.removeNode(node);
                continue;
            }

            node.data.jobType = jobType;
        }
    }

    protected abstract createDepGraphContext(
        entryFiles: Set<string>,
        fileToModule: Map<string, ModuleInfo>,
        depMap: DependencyFileMap
    ): DepGraphContext;

    public getGraph(
        entryFiles: Set<string>,
        fileToModule: Map<string, ModuleInfo>,
        moduleInfos: Map<string, ModuleInfo>,
        outputs: string[]
    ): Graph<CompileJobInfo> {
        // Step 1: Generate full dependency map
        const dependencyMap = this.generateDependencyMap(entryFiles, Array.from(moduleInfos.values()), fileToModule);
        for (const file of entryFiles) {
            outputs.push(dependencyMap.outputMatching[file]);
        }

        // Step 2: Build raw dependency graph
        const depGraphContext = this.createDepGraphContext(entryFiles, fileToModule, dependencyMap);
        this.statsRecorder.record(formEvent(DepAnalyzerEvent.CREATE_GRAPH));
        const dependencyGraph = this.createDependencyGraph(
            depGraphContext.entryFiles,
            depGraphContext.fileToModule,
            depGraphContext.dependencyMap
        );
        this.verifyAndDumpGraph(dependencyGraph, 'graph.dot');

        // Step 3: First graph filter preparation
        this.statsRecorder.record(formEvent(DepAnalyzerEvent.FIRST_FILTER_GRAPH));
        const nodeMerger = (lhs: GraphNode<CompileJobInfo>, rhs: GraphNode<CompileJobInfo>): CompileJobInfo => {
            let files: FileInfo[] = [];
            const appendFiles = (job: JobInfo): void => {
                if (job.contentType === JobContentType.FILE) {
                    files.push(job.content as FileInfo);
                } else {
                    files = files.concat(job.content as FileInfo[]);
                }
            };
            appendFiles(lhs.data);
            appendFiles(rhs.data);

            return {
                contentType: JobContentType.CLUSTER,
                content: files,
                arktsConfig: lhs.data.arktsConfig,
                moduleName: lhs.data.moduleName,
                moduleRoot: lhs.data.moduleRoot,
                declgenConfig: {
                    output: lhs.data.declgenConfig.output
                },
                jobType: lhs.data.jobType | rhs.data.jobType
            };
        };

        const cycleMerger = (lhs: GraphNode<CompileJobInfo>, rhs: GraphNode<CompileJobInfo>): CompileJobInfo => {
            const lModuleName = lhs.data.moduleName;
            const rModuleName = rhs.data.moduleName;
            if (lModuleName !== rModuleName) {
                throw new DriverError(
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_DEPENDENCY_ANALYZE_FAIL,
                        'Cyclic dependency between modules found.',
                        `Module cycle: ${lModuleName} <---> ${rModuleName}`
                    )
                );
            }
            return nodeMerger(lhs, rhs);
        };

        // Step 4: Collapse cycle nodes
        this.statsRecorder.record(formEvent(DepAnalyzerEvent.COLLAPSE_CYCLES));
        Graph.collapseCycles(dependencyGraph, cycleMerger);
        this.verifyAndDumpGraph(dependencyGraph, 'graph.collapsed.dot');

        // Step 5: Second graph filter, remove unchanged nodes
        this.statsRecorder.record(formEvent(DepAnalyzerEvent.SECOND_FILTER_GRAPH));
        this.filterGraph(dependencyGraph, fileToModule);
        this.verifyAndDumpGraph(dependencyGraph, 'graph.filtered.clusters.dot');

        // Step 6: Merge nodes into cluster if cluster build enabled
        if (this.clusteredBuild) {
            const mainModule = Array.from(moduleInfos.values()).find(module => module.isMainModule)!;
            this.statsRecorder.record(formEvent(DepAnalyzerEvent.CLUSTER_GRAPH));
            const nodeIds = Graph.topologicalSort(dependencyGraph);

            while (nodeIds.length > 0) {
                let cluster = dependencyGraph.getNodeById(nodeIds.shift()!);
                cluster.data.arktsConfig = mainModule.arktsConfigFile;
                cluster.data.moduleName = mainModule.packageName;

                for (let counter = 0; counter < CLUSTER_FILES_THRESHOLD - 1 && nodeIds.length > 0; counter++) {
                    const nodeToMerge = dependencyGraph.getNodeById(nodeIds.shift()!);
                    cluster = dependencyGraph.mergeNodes(cluster, nodeToMerge, nodeMerger);
                }
            }

            this.verifyAndDumpGraph(dependencyGraph, 'graph.clustered.dot');
        }

        this.statsRecorder.record(formEvent(DepAnalyzerEvent.SAVE_HASH));
        this.saveHashCache();
        this.statsRecorder.record(RecordEvent.END);
        this.statsRecorder.writeSumSingle();

        return dependencyGraph;
    }
}