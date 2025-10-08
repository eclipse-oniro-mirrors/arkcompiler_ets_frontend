/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
    DECL_ETS_SUFFIX,
    MERGED_CLUSTER_FILE,
    FILE_HASH_CACHE,
    ABC_SUFFIX,
    CLUSTER_FILES_TRESHOLD
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
    CompileJobType
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

function formEvent(event: DepAnalyzerEvent) {
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
    private entryFiles: Set<string>;
    private filesHashCache: Record<string, string>;

    constructor(buildConfig: BuildConfig) {
        this.logger = Logger.getInstance();

        this.entryFiles = new Set<string>(buildConfig.compileFiles);

        this.cacheDir = buildConfig.cachePath;
        this.outputDir = path.join(buildConfig.cachePath, DEP_ANALYZER_DIR);
        ensureDirExists(this.outputDir);
        this.binPath = buildConfig.dependencyAnalyzerPath!;

        this.hashCacheFile = path.resolve(buildConfig.cachePath, FILE_HASH_CACHE);
        this.filesHashCache = this.loadHashCache();

        this.statsRecorder = new StatisticsRecorder(
            path.resolve(this.cacheDir, BS_PERF_FILE_NAME),
            buildConfig.recordType,
            `Dependency analyzer`
        );

        this.dumpGraph = buildConfig.dumpDependencyGraph ?? false;
        this.clusteredBuild = buildConfig.clusteredBuild ?? false;
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
        let resArkTSConfig: ArkTSConfig = cloneDeep(ArkTSConfigGenerator.getInstance().getArktsConfigByPackageName(mainModule.packageName)!)
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

    private formExecCmd(input: string, output: string, config: string): string {
        let cmd = [path.resolve(this.binPath)];
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
            dependencies: {}
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

        this.logger.printDebug(`filtered dependency map: ${JSON.stringify(resDependencyMap, null, 1)}`)
        return resDependencyMap;
    }

    private get mergedArktsConfigPath() {
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

        const dependencyGraphNodes: GraphNode<CompileJobInfo>[] = [];
        for (const file of entryFiles) {
            const module: ModuleInfo = fileToModule.get(file)!
            const output: string = path.resolve(
                this.cacheDir, module.packageName,
                changeFileExtension(
                    path.relative(module.moduleRootPath, file),
                    ABC_SUFFIX
                )
            );
            const node = new GraphNode<CompileJobInfo>(computeHash(file), {
                fileInfo: {
                    input: file,
                    output: output,
                    arktsConfig: module.arktsConfigFile,
                    moduleName: module.packageName,
                    moduleRoot: module.moduleRootPath,
                },
                fileList: [file],
                declgenConfig: {
                    output: module.declgenV2OutPath!
                },
                type: CompileJobType.DECL_ABC
            });

            for (const dependency of dependencyMap.dependencies[file]) {
                node.predecessors.add(computeHash(dependency));
            }

            for (const dependant of dependencyMap.dependants[file]) {
                node.descendants.add(computeHash(dependant));
            }
            dependencyGraphNodes.push(node);
        }

        const dependencyGraph: Graph<CompileJobInfo> = Graph.createGraphFromNodes(dependencyGraphNodes);
        dependencyGraph.verify();
        if (this.dumpGraph) {
            fs.writeFileSync(path.resolve(this.cacheDir, 'graph.dot'), dotGraphDump(dependencyGraph), 'utf-8');
        }

        this.statsRecorder.record(formEvent(DepAnalyzerEvent.COLLAPSE_CYCLES));
        const dataMerger = (lhs: GraphNode<CompileJobInfo>, rhs: GraphNode<CompileJobInfo>): CompileJobInfo => {
            const outputAbc = path.resolve(
                this.cacheDir, 'clusters', computeHash([lhs.id, rhs.id].join('|')), MERGED_CLUSTER_FILE,
            );
            return {
                fileInfo: {
                    // In clusters this field is meaningless
                    input: lhs.data.fileInfo.input,
                    output: outputAbc,
                    arktsConfig: lhs.data.fileInfo.arktsConfig,
                    moduleName: lhs.data.fileInfo.moduleName,
                    moduleRoot: lhs.data.fileInfo.moduleRoot,
                },
                fileList: [...lhs.data.fileList, ...rhs.data.fileList],
                declgenConfig: {
                    output: lhs.data.declgenConfig.output
                },
                type: CompileJobType.DECL_ABC
            }
        };
        const cycleMerger = (lhs: GraphNode<CompileJobInfo>, rhs: GraphNode<CompileJobInfo>): CompileJobInfo => {
            if ((lhs.data.fileInfo.moduleName != rhs.data.fileInfo.moduleName) ||
                (lhs.data.fileInfo.moduleRoot != rhs.data.fileInfo.moduleRoot)) {
                throw new DriverError(
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_DEPENDENCY_ANALYZE_FAIL,
                        'Cyclic dependency between modules found.',
                        `Module cycle: ${lhs.data.fileInfo.moduleName} <---> ${rhs.data.fileInfo.moduleName}`)
                )
            }
            return dataMerger(lhs, rhs);
        }
        Graph.collapseCycles(dependencyGraph, cycleMerger);

        dependencyGraph.verify();
        if (this.dumpGraph) {
            fs.writeFileSync(path.resolve(this.cacheDir, 'graph.collapsed.dot'), dotGraphDump(dependencyGraph), 'utf-8');
        }

        // NOTE: some workaround to gather all outputs
        // NOTE: likely to be refactored
        dependencyGraph.nodes.forEach((node: GraphNode<CompileJobInfo>) => {
            outputs.push(node.data.fileInfo.output);
        });

        this.statsRecorder.record(formEvent(DepAnalyzerEvent.FILTER_GRAPH));
        this.filterGraph(dependencyGraph);

        dependencyGraph.verify();
        if (this.dumpGraph) {
            fs.writeFileSync(path.resolve(this.cacheDir, 'graph.filtered.dot'), dotGraphDump(dependencyGraph), 'utf-8');
        }

        if (this.clusteredBuild) {
            this.statsRecorder.record(formEvent(DepAnalyzerEvent.CLUSTER_GRAPH));
            const nodeIds: string[] = Graph.topologicalSort(dependencyGraph);
            let currentClusterNode = dependencyGraph.getNodeById(nodeIds.shift()!);
            for (const nodeId of nodeIds) {
                const node = dependencyGraph.getNodeById(nodeId);
                if (currentClusterNode.descendants.has(nodeId) &&
                    (currentClusterNode.data.fileList.length + node.data.fileList.length) < CLUSTER_FILES_TRESHOLD) {
                    currentClusterNode = dependencyGraph.mergeNodes(currentClusterNode, node, dataMerger);
                } else {
                    currentClusterNode = node;
                }
            }

            dependencyGraph.verify();
            if (this.dumpGraph) {
                fs.writeFileSync(path.resolve(this.cacheDir, 'graph.clustered.dot'), dotGraphDump(dependencyGraph), 'utf-8');
            }
        }

        this.statsRecorder.record(formEvent(DepAnalyzerEvent.SAVE_HASH));
        this.saveHashCache();
        this.statsRecorder.record(RecordEvent.END);
        this.statsRecorder.writeSumSingle();

        return dependencyGraph;
    }

    private filterGraph(graph: Graph<CompileJobInfo>) {
        for (const nodeId of Graph.topologicalSort(graph)) {
            const node = graph.getNodeById(nodeId);
            if (node.predecessors.size != 0) {
                // Still has dependencies, so do not remove the node
                // Update file hashes
                node.data.fileList.map((file: string) => updateFileHash(file, this.filesHashCache));
                node.data.type = CompileJobType.DECL_ABC;
                continue;
            }

            if (node.data.fileList.length > 1) {
                let shouldBeCompiled: boolean = false;
                for (const file of node.data.fileList) {
                    shouldBeCompiled = updateFileHash(file, this.filesHashCache)
                        || shouldBeUpdated(file, node.data.fileInfo.output)
                        || shouldBeCompiled
                }

                if (!shouldBeCompiled) {
                    this.logger.printDebug(`Skipping cluster ${node.id} compilation`);
                    graph.removeNode(node);
                }

                node.data.type = CompileJobType.DECL_ABC;
                continue;
            }

            const input = node.data.fileInfo.input;
            const outputAbc = node.data.fileInfo.output;
            const outputDecl = changeFileExtension(outputAbc, DECL_ETS_SUFFIX);

            const hashChanged: boolean = updateFileHash(input, this.filesHashCache);
            const genDecl: boolean = hashChanged || shouldBeUpdated(input, outputDecl);
            const compileAbc: boolean = hashChanged || shouldBeUpdated(input, outputAbc);

            if (!compileAbc && !genDecl) {
                this.logger.printDebug(`Skipping file ${input} compilation`)
                graph.removeNode(node);
            }

            node.data.type &= CompileJobType.NONE;
            if (genDecl) {
                node.data.type |= CompileJobType.DECL;
            }

            if (compileAbc) {
                node.data.type |= CompileJobType.ABC;
            }
        }
    }
}
