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

import {
    ARKTSCONFIG_JSON_FILE,
    DEP_ANALYZER_DIR,
    DEP_ANALYZER_INPUT_FILE,
    DEP_ANALYZER_OUTPUT_FILE,
    FILE_HASH_CACHE,
    ETSCACHE_SUFFIX,
    CLUSTER_FILES_TRESHOLD,
    ENABLE_CLUSTERS,
    ENABLE_DECL_FILE_CACHE
} from './pre_define';

import {
    changeFileExtension,
    shouldBeUpdated,
    updateFileHash,
    ensureDirExists,
    ensurePathExists,
    isMac
} from './util/utils';

import {
    BuildConfig,
    ModuleInfo,
    CompileJobInfo,
    CompileJobType,
    JobContentType,
    JobInfo,
    FileInfo,
    OHOS_MODULE_TYPE,
    isHarOrHsp
} from './types'

import {
    Logger,
    LogDataFactory
} from './logger';

import {
    BS_PERF_FILE_NAME,
    StatisticsRecorder,
    RecordEvent
} from './util/statsRecorder'

import { ErrorCode, DriverError } from './util/error';

import { ArkTSConfigGenerator, ArkTSConfig } from './build/generate_arktsconfig';

import { computeHash } from './util/utils'

import { Graph, GraphNode } from './util/graph';

import { dotGraphDump } from './util/dotGraphDump';

import cloneDeep from 'lodash.clonedeep'


export interface DependencyFileMap {
    dependants: {
        [filePath: string]: string[];
    };
    dependencies: {
        [filePath: string]: string[];
    };
    outputMatching: {
        [filePath: string]: string;
    }
}

enum DepAnalyzerEvent {
    GEN_DEPENDENCY_MAP = 'Generate dependency map (spawn exec tool)',
    CREATE_GRAPH = 'Create graph',
    COLLAPSE_CYCLES = 'Collapse cycles in graph',
    FILTER_GRAPH = 'Filter jobs to build',
    CLUSTER_GRAPH = 'Merge jobs into clusters',
    SAVE_HASH = 'Save source files\' hashes'
}

function formEvent(event: DepAnalyzerEvent): string {
    return '[Dependency analyzer] ' + event;
}

export class DependencyAnalyzer {

    private readonly logger: Logger;
    private readonly binPath: string;
    private readonly outputDir: string;
    private readonly cacheDir: string;
    private readonly hashCacheFile: string;
    private readonly statsRecorder: StatisticsRecorder;
    private readonly dumpGraph: boolean = false;
    private readonly clusteredBuild: boolean = false;
    private readonly mainModuleType: OHOS_MODULE_TYPE;
    private entryFiles: Set<string>;
    private filesHashCache: Record<string, string>;

    constructor(buildConfig: BuildConfig, clusteredBuild: boolean = ENABLE_CLUSTERS) {
        this.logger = Logger.getInstance();

        this.entryFiles = new Set<string>(buildConfig.compileFiles);

        this.cacheDir = buildConfig.cachePath;
        this.outputDir = path.join(buildConfig.cachePath, DEP_ANALYZER_DIR);
        ensureDirExists(this.outputDir);
        this.binPath = buildConfig.dependencyAnalyzerPath!;

        this.hashCacheFile = path.resolve(buildConfig.cachePath, DEP_ANALYZER_DIR, FILE_HASH_CACHE);
        this.filesHashCache = this.loadHashCache();

        this.statsRecorder = new StatisticsRecorder(
            path.resolve(this.cacheDir, BS_PERF_FILE_NAME),
            buildConfig.recordType,
            'Dependency analyzer'
        );

        this.clusteredBuild = clusteredBuild;
        this.dumpGraph = buildConfig.dumpDependencyGraph ?? false;
        this.mainModuleType = buildConfig.moduleType;
    }

    private loadHashCache(): Record<string, string> {
        try {
            if (!fs.existsSync(this.hashCacheFile)) {
                this.logger.printDebug(`no hash cache file: ${this.hashCacheFile}`)
                return {};
            }

            const cacheContent: string = fs.readFileSync(this.hashCacheFile, 'utf-8');
            this.logger.printDebug(`cacheContent: ${cacheContent}`)
            const cacheData: Record<string, string> = JSON.parse(cacheContent);
            const filteredCache: Record<string, string> = Object.fromEntries(
                Object.entries(cacheData).filter(([file]) => this.entryFiles.has(file))
            );
            return filteredCache;
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

    private generateMergedArktsConfig(modules: Array<ModuleInfo>, outputPath: string): void {

        let mainModule = modules.find((module) => module.isMainModule)!
        // NOTE: create new temporary arktsconfig for dependency analyzer
        let resArkTSConfig: ArkTSConfig = cloneDeep(this.getArktsConfigByPackageName(mainModule.packageName)!)
        modules.forEach((module) => {
            if (module.isMainModule) {
                return;
            }
            resArkTSConfig.mergeArktsConfig(
                ArkTSConfigGenerator.getInstance().getArktsConfigByPackageName(module.packageName)!
            )
        });

        fs.writeFileSync(outputPath, JSON.stringify(resArkTSConfig.object, null, 2));
    }

    private getArktsConfigByPackageName(name: string): ArkTSConfig | undefined {
        return ArkTSConfigGenerator.getInstance().getArktsConfigByPackageName(name)
    }

    private formExecCmd(input: string, output: string, config: string): string {
        let cmd = [];
        cmd.push('"' + path.resolve(this.binPath) + '"');
        cmd.push('@' + '"' + input + '"');
        cmd.push('--arktsconfig=' + '"' + config + '"');
        cmd.push('--output=' + '"' + output + '"');
        let res: string = cmd.join(' ');
        if (isMac()) {
            const loadLibrary = 'DYLD_LIBRARY_PATH=' + '"' + process.env.DYLD_LIBRARY_PATH + '"';
            res = loadLibrary + ' ' + res;
        }
        return res;
    }

    private filterDependencyMap(
        dependencyMap: DependencyFileMap,
        entryFiles: Set<string>
    ): DependencyFileMap {
        let resDependencyMap: DependencyFileMap = {
            dependants: {},
            dependencies: {},
            outputMatching: {}
        }

        // Filter files by entryFiles
        Object.entries(dependencyMap.dependencies).forEach(([file, dependencies]: [string, string[]]) => {
            if (!entryFiles.has(file)) {
                return
            }
            resDependencyMap.dependencies[file] = [...dependencies].filter((dependency: string) => {
                return entryFiles.has(dependency)
            })
        })
        Object.entries(dependencyMap.dependants).forEach(([file, dependants]: [string, string[]]) => {
            if (!entryFiles.has(file)) {
                return
            }
            resDependencyMap.dependants[file] = [...dependants].filter((dependant: string) => {
                return entryFiles.has(dependant)
            })
        })

        resDependencyMap.outputMatching = dependencyMap.outputMatching;

        this.logger.printDebug(`filtered dependency map: ${JSON.stringify(resDependencyMap, null, 1)}`)
        return resDependencyMap;
    }

    private get mergedArktsConfigPath(): string {
        return path.join(this.outputDir, ARKTSCONFIG_JSON_FILE);
    }

    private generateDependencyMap(
        entryFiles: Set<string>,
        modules: Array<ModuleInfo>
    ): DependencyFileMap {
        const inputFile: string = path.join(this.outputDir, DEP_ANALYZER_INPUT_FILE);
        const outputFile: string = path.join(this.outputDir, DEP_ANALYZER_OUTPUT_FILE);
        const arktsConfigPath: string = this.mergedArktsConfigPath;

        let depAnalyzerInputFileContent: string = Array.from(entryFiles).join(os.EOL);
        fs.writeFileSync(inputFile, depAnalyzerInputFileContent);

        this.generateMergedArktsConfig(modules, arktsConfigPath)

        let execCmd = this.formExecCmd(inputFile, outputFile, arktsConfigPath)
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
                )
            }
        }
        const fullDependencyMap: DependencyFileMap = JSON.parse(fs.readFileSync(outputFile, 'utf-8'));
        Object.keys(fullDependencyMap.dependants).forEach((file: string) => {
            if (!(file in fullDependencyMap.dependencies)) {
                fullDependencyMap.dependencies[file] = [];
            }
        });

        this.logger.printDebug(`full dependency map: ${JSON.stringify(fullDependencyMap, null, 1)}`)
        return this.filterDependencyMap(fullDependencyMap, entryFiles);
    }

    private verifyAndDumpGraph(graph: Graph<CompileJobInfo>, output: string): void {
        graph.verify();
        if (this.dumpGraph) {
            fs.writeFileSync(path.resolve(this.cacheDir, output), dotGraphDump(graph), 'utf-8');
        }
    }

    private createDependencyGraph(entryFiles: Set<string>, fileToModule: Map<string, ModuleInfo>, dependencyMap: DependencyFileMap) {
        const dependencyGraphNodes: GraphNode<CompileJobInfo>[] = [];
        /*
         * Althrough we will set jobType in filterGraph again , but when there is a cycle in the dependency graph
         * we should recompile abc for hap 
         * recompile abc and regenerate decl for har and hsp
         */
        let jobType = CompileJobType.ABC;
        if(isHarOrHsp(this.mainModuleType) && ENABLE_DECL_FILE_CACHE) {
            jobType |= CompileJobType.DECL;
        }

        for (const file of entryFiles) {
            const module: ModuleInfo = fileToModule.get(file)!
            const node = new GraphNode<CompileJobInfo>(computeHash(file), {
                contentType: JobContentType.FILE,
                content: {
                    input: file,
                    output: dependencyMap.outputMatching[file],
                },
                arktsConfig: module.arktsConfigFile,
                moduleName: module.packageName,
                moduleRoot: module.moduleRootPath,
                declgenConfig: {
                    output: module.declgenV2OutPath!
                },
                jobType: jobType
            });
            if (dependencyMap.dependencies[file]) {
                for (const dependency of dependencyMap.dependencies[file]) {
                    node.predecessors.add(computeHash(dependency));
                }
            }
            if (dependencyMap.dependants[file]) {
                for (const dependant of dependencyMap.dependants[file]) {
                    node.descendants.add(computeHash(dependant));
                }
            }
            dependencyGraphNodes.push(node);
        }

        return Graph.createGraphFromNodes(dependencyGraphNodes);
    }

    public getGraph(
        entryFiles: Set<string>,
        fileToModule: Map<string, ModuleInfo>,
        moduleInfos: Map<string, ModuleInfo>,
        outputs: string[]
    ): Graph<CompileJobInfo> {
        this.statsRecorder.record(formEvent(DepAnalyzerEvent.GEN_DEPENDENCY_MAP));
        const dependencyMap: DependencyFileMap =
            this.generateDependencyMap(entryFiles, Array.from(moduleInfos.values()));

        this.statsRecorder.record(formEvent(DepAnalyzerEvent.CREATE_GRAPH));
        const dependencyGraph: Graph<CompileJobInfo> =
            this.createDependencyGraph(entryFiles, fileToModule, dependencyMap);
        this.verifyAndDumpGraph(dependencyGraph, 'graph.dot');

        // NOTE(mshimenkov): Collect output files to pass them to the linker later
        dependencyGraph.nodes.forEach((node: GraphNode<CompileJobInfo>) => {
            outputs.push(dependencyMap.outputMatching[(node.data.content as FileInfo).input]);
        });

        this.statsRecorder.record(formEvent(DepAnalyzerEvent.FILTER_GRAPH));
        this.filterGraph(dependencyGraph, fileToModule);
        this.verifyAndDumpGraph(dependencyGraph, 'graph.filtered.dot');

        const nodeMerger = (lhs: GraphNode<CompileJobInfo>, rhs: GraphNode<CompileJobInfo>): CompileJobInfo => {
            let files: FileInfo[] = []
            const appendFiles = (job: JobInfo) => {
                if (job.contentType === JobContentType.FILE) {
                    files.push(job.content as FileInfo);
                } else {
                    files = files.concat(job.content as FileInfo[]);
                }
            }
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
            }
        };
        const cycleMerger = (lhs: GraphNode<CompileJobInfo>, rhs: GraphNode<CompileJobInfo>): CompileJobInfo => {
            const lModuleName: string = lhs.data.moduleName;
            const rModuleName: string = rhs.data.moduleName;
            if (lModuleName !== rModuleName)
                throw new DriverError(
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_DEPENDENCY_ANALYZE_FAIL,
                        'Cyclic dependency between modules found.',
                        `Module cycle: ${lModuleName} <---> ${rModuleName}`)
                )
            return nodeMerger(lhs, rhs);
        }
        this.statsRecorder.record(formEvent(DepAnalyzerEvent.COLLAPSE_CYCLES));
        Graph.collapseCycles(dependencyGraph, cycleMerger);
        this.verifyAndDumpGraph(dependencyGraph, 'graph.collapsed.dot');

        if (this.clusteredBuild) {
            let mainModule = Array.from(moduleInfos.values()).find((module) => module.isMainModule)!
            this.statsRecorder.record(formEvent(DepAnalyzerEvent.CLUSTER_GRAPH));
            const nodeIds: string[] = Graph.topologicalSort(dependencyGraph);
            while (nodeIds.length > 0) {
                let cluster = dependencyGraph.getNodeById(nodeIds.shift()!);
                cluster.data.arktsConfig = mainModule.arktsConfigFile;
                cluster.data.moduleName = mainModule.packageName;
                for (let counter = 0; counter < CLUSTER_FILES_TRESHOLD - 1 && nodeIds.length > 0; counter++) {
                    let nodeToMerge = dependencyGraph.getNodeById(nodeIds.shift()!);
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

    private filterGraph(graph: Graph<CompileJobInfo>, fileToModule: Map<string, ModuleInfo>): void {
        for (const nodeId of Graph.topologicalSort(graph)) {
            const node = graph.getNodeById(nodeId);
            if (node.data.contentType !== JobContentType.FILE) {
                throw new DriverError(
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_GRAPH_ERROR,
                        'Corrupted graph: graph should contain only \'File\' nodes before filtering.'
                    )
                )
            }
            if (node.predecessors.size !== 0) {
                // Still has dependencies, so do not remove the node
                // Just update file hashes
                updateFileHash((node.data.content as FileInfo).input, this.filesHashCache);
                continue;
            }

            const fi = node.data.content as FileInfo;
            const input = fi.input;
            const outputAbc = fi.output;
            const outputDecl = changeFileExtension(outputAbc, ETSCACHE_SUFFIX);
            const hashChanged: boolean = updateFileHash(input, this.filesHashCache);
            const compileAbc: boolean = hashChanged || shouldBeUpdated(input, outputAbc);
            let genDecl: boolean = false;
            if(isHarOrHsp(this.mainModuleType)) {
                genDecl = ENABLE_DECL_FILE_CACHE && (hashChanged || shouldBeUpdated(input, outputDecl));
            }

            if (!compileAbc && !genDecl) {
                this.logger.printDebug(`Skipping file ${input} compilation`);
                graph.removeNode(node);
                continue;
            }

            node.data.jobType &= CompileJobType.NONE;
            if (genDecl) {
                node.data.jobType |= CompileJobType.DECL;
            }

            if (compileAbc) {
                node.data.jobType |= CompileJobType.ABC;
            }
        }
    }
}

