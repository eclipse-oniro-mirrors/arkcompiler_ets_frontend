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
    ABC_SUFFIX,
    MERGED_CYCLE_FILE,
    FILE_HASH_CACHE
} from './pre_define';

import {
    changeFileExtension,
    shouldBeCompiled,
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

import { ErrorCode, DriverError } from './util/error';

import { ArkTSConfigGenerator, ArkTSConfig } from './build/generate_arktsconfig';

import { computeHash } from './util/utils'

import cloneDeep from 'lodash.clonedeep'

export interface DependencyFileMap {
    dependants: {
        [filePath: string]: string[];
    };
    dependencies: {
        [filePath: string]: string[];
    }
}

export class DependencyAnalyzer {

    private readonly logger: Logger;
    private readonly binPath: string;
    private readonly outputDir: string;
    private readonly cacheDir: string;
    private readonly hashCacheFile: string;
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

    private generateDependencyMap(
        entryFiles: Set<string>,
        modules: Array<ModuleInfo>
    ): DependencyFileMap {
        const inputFile: string = path.join(this.outputDir, DEP_ANALYZER_INPUT_FILE);
        const outputFile: string = path.join(this.outputDir, DEP_ANALYZER_OUTPUT_FILE);
        const arktsConfigPath: string = path.join(this.outputDir, ARKTSCONFIG_JSON_FILE);

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

    private findStronglyConnectedComponents(fileMap: DependencyFileMap): Map<string, Set<string>> {
        const adjacencyList: Record<string, string[]> = {};
        const reverseAdjacencyList: Record<string, string[]> = {};
        const allNodes = new Set<string>();

        for (const node in fileMap.dependencies) {
            allNodes.add(node);
            fileMap.dependencies[node].forEach(dep => allNodes.add(dep));
        }
        for (const node in fileMap.dependants) {
            allNodes.add(node);
            fileMap.dependants[node].forEach(dep => allNodes.add(dep));
        }

        allNodes.forEach(node => {
            adjacencyList[node] = fileMap.dependencies[node] || [];
            reverseAdjacencyList[node] = fileMap.dependants[node] || [];
        });

        const visited = new Set<string>();
        const order: string[] = [];

        function dfs(node: string): void {
            visited.add(node);
            for (const neighbor of adjacencyList[node]) {
                if (!visited.has(neighbor)) {
                    dfs(neighbor);
                }
            }
            order.push(node);
        }

        allNodes.forEach(node => {
            if (!visited.has(node)) {
                dfs(node);
            }
        });

        visited.clear();
        const components = new Map<string, Set<string>>();

        function reverseDfs(node: string, component: Set<string>): void {
            visited.add(node);
            component.add(node);
            for (const neighbor of reverseAdjacencyList[node]) {
                if (!visited.has(neighbor)) {
                    reverseDfs(neighbor, component);
                }
            }
        }

        for (let i = order.length - 1; i >= 0; i--) {
            const node = order[i];
            if (!visited.has(node)) {
                const component = new Set<string>();
                reverseDfs(node, component);
                if (component.size > 1) {
                    const sortedFiles = Array.from(component).sort();
                    const componentId = computeHash(sortedFiles.join('|'));
                    components.set(componentId, component);
                }
            }
        }

        this.logger.printDebug(`Found components: ${JSON.stringify([...components], null, 1)}`)
        return components;
    }

    private verifyModuleCyclicDependency(files: string[], fileToModule: Map<string, ModuleInfo>) {
        const modules = files.map((file: string) => {
            const module = fileToModule.get(file)
            if (!module) {
                throw new DriverError(
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_DEPENDENCY_ANALYZE_FAIL,
                        `Failed to find module for file ${file}.`,
                    )
                )
            }
            return module
        })
        const set = new Set(modules.map((module: ModuleInfo) => module.packageName))
        if (set.size > 1) {
            throw new DriverError(
                LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_DEPENDENCY_ANALYZE_FAIL,
                    'Cyclic dependency between modules found.',
                    "Module cycle: " + Array.from(set).join(" <---> ")
                )
            )
        }
    }

    private createCycleJob(
        jobs: Record<string, CompileJobInfo>,
        cycle: Set<string>,
        cycleId: string,
        dependencyMap: DependencyFileMap,
        fileToCycle: Map<string, string>,
        fileToModule: Map<string, ModuleInfo>
    ) {
        const cycleFileList = Array.from(cycle)
        const cycleDependencies = cycleFileList.map(
            (file: string) => this.collectJobDependencies(file, dependencyMap, fileToCycle)
        ).reduce((acc: Set<string>, curr: Set<string>) => new Set([...acc, ...curr]))

        const cycleDependants: Set<string> = cycleFileList.map(
            (file: string) => this.collectJobDependants(file, dependencyMap, fileToCycle)
        ).reduce((acc: Set<string>, curr: Set<string>) => new Set([...acc, ...curr]))

        const inputFile = cycleFileList[0];
        const outputFile = path.resolve(this.cacheDir, "cycles", cycleId, MERGED_CYCLE_FILE);
        ensurePathExists(outputFile);
        const module: ModuleInfo = fileToModule.get(inputFile)!
        const arktsConfigFile = module.arktsConfigFile

        jobs[cycleId] = {
            id: cycleId,
            fileList: cycleFileList,
            jobDependencies: Array.from(cycleDependencies),
            jobDependants: Array.from(cycleDependants),
            fileInfo: {
                input: inputFile,
                output: outputFile,
                arktsConfig: arktsConfigFile,
                moduleName: module.packageName,
                moduleRoot: module.moduleRootPath,
            },
            declgenConfig: {
                output: module.declgenV2OutPath!
            },
            type: CompileJobType.DECL_ABC
        }
        this.logger.printDebug(`Created job for cycle: ${JSON.stringify(jobs[cycleId], null, 1)}`)
    }

    private filterCollectedJobs(jobs: Record<string, CompileJobInfo>): Record<string, CompileJobInfo> {
        let filteredJobs: Record<string, CompileJobInfo> = {}

        const addJobRecursively = (jobId: string) => {
            const job: CompileJobInfo = jobs[jobId];
            filteredJobs[jobId] = job;
            for (const dependant of job.jobDependants) {
                addJobRecursively(dependant);
            }
        }

        const skipJob = (jobId: string) => {
            const job: CompileJobInfo = jobs[jobId];
            job.jobDependants.forEach((dependantId: string) => {
                let dependant = jobs[dependantId]
                dependant.jobDependencies = dependant.jobDependencies.filter((dependency: string) => dependency != jobId)
            })
        }

        for (const [jobId, jobInfo] of Object.entries(jobs)) {

            if (filteredJobs[jobId]) {
                // Already added this job
                continue;
            }

            if (jobInfo.fileList.length > 1) {
                let shouldCompile: boolean = false;
                for (const file of jobInfo.fileList) {
                    shouldCompile = updateFileHash(file, this.filesHashCache) || shouldCompile
                }

                if (!shouldCompile) {
                    this.logger.printDebug(`Skipping cycle ${jobId} compilation`)

                    skipJob(jobId);
                    continue;
                }

                // For now no decl files are generated for cycles
                jobInfo.type &= CompileJobType.ABC;
            } else {
                const inputFilePath = jobInfo.fileInfo.input;
                const outputFilePath = jobInfo.fileInfo.output;
                const outputDeclFilePath = changeFileExtension(outputFilePath, DECL_ETS_SUFFIX)
                ensurePathExists(outputDeclFilePath);

                const hashChanged: boolean = updateFileHash(inputFilePath, this.filesHashCache)
                const compileAbc: boolean = hashChanged || shouldBeCompiled(inputFilePath, outputFilePath)
                const genDecl: boolean = hashChanged || shouldBeCompiled(inputFilePath, outputDeclFilePath)

                if (!compileAbc && !genDecl) {
                    this.logger.printDebug(`Skipping file ${inputFilePath} compilation`)

                    skipJob(jobId);
                    continue;
                }

                jobInfo.type &= CompileJobType.NONE;

                if (genDecl) {
                    jobInfo.type |= CompileJobType.DECL;
                }

                if (compileAbc) {
                    jobInfo.type |= CompileJobType.ABC;
                }
            }

            addJobRecursively(jobId);
        }

        return filteredJobs;
    }

    public collectJobs(
        entryFiles: Set<string>,
        fileToModule: Map<string, ModuleInfo>,
        moduleInfos: Map<string, ModuleInfo>
    ): Record<string, CompileJobInfo> {
        let jobs: Record<string, CompileJobInfo> = {};

        const dependencyMap: DependencyFileMap =
            this.generateDependencyMap(entryFiles, Array.from(moduleInfos.values()));

        const stronglyConnectedComponents: Map<string, Set<string>> = this.findStronglyConnectedComponents(dependencyMap);
        const fileToCycleMap: Map<string, string> = new Map<string, string>();

        // First iterate to check Module Cyclic Dependencies and fill fileToCycleMap
        stronglyConnectedComponents.forEach((component: Set<string>, componentId: string) => {
            this.verifyModuleCyclicDependency(Array.from(component), fileToModule);
            component.forEach((file) => {
                fileToCycleMap.set(file, componentId);
            });
        });
        this.logger.printDebug(`Found stronglyConnectedComponents: ${JSON.stringify([...stronglyConnectedComponents], null, 1)}`)
        this.logger.printDebug(`fileToCycleMap: ${JSON.stringify([...fileToCycleMap], null, 1)}`)

        // Second iterate to create jobs to compile cycles
        stronglyConnectedComponents.forEach((component: Set<string>, componentId: string) => {
            this.createCycleJob(jobs, component, componentId, dependencyMap, fileToCycleMap, fileToModule)
        });

        entryFiles.forEach((file: string) => {
            const isInCycle: boolean = fileToCycleMap.has(file)
            if (isInCycle) {
                return;
            }

            const jobId: string = this.getJobId(file)

            const module: ModuleInfo = fileToModule.get(file)!
            const outputFile = path.resolve(this.cacheDir, module.packageName,
                changeFileExtension(
                    path.relative(module.moduleRootPath, file),
                    ABC_SUFFIX
                )
            )
            ensurePathExists(outputFile);
            const arktsConfigFile: string = module.arktsConfigFile

            const jobDependencies: Set<string> = this.collectJobDependencies(file, dependencyMap, fileToCycleMap);
            const jobDependants: Set<string> = this.collectJobDependants(file, dependencyMap, fileToCycleMap);
            jobs[jobId] = {
                id: jobId,
                fileList: [file],
                jobDependencies: [...jobDependencies],
                jobDependants: [...jobDependants],
                fileInfo: {
                    input: file,
                    output: outputFile,
                    arktsConfig: arktsConfigFile,
                    moduleName: module.packageName,
                    moduleRoot: module.moduleRootPath,
                },
                declgenConfig: {
                    output: module.declgenV2OutPath!
                },
                type: CompileJobType.DECL_ABC
            }
            this.logger.printDebug(`Created job: ${JSON.stringify(jobs[jobId], null, 1)}`)
        });

        jobs = this.filterCollectedJobs(jobs);
        this.saveHashCache();
        this.logger.printDebug(`Collected jobs: ${JSON.stringify(jobs, null, 1)}`)

        return jobs;
    }

    private collectJobDependencies(
        file: string,
        dependencyMap: DependencyFileMap,
        fileToCycleMap: Map<string, string>
    ): Set<string> {
        const fileDependencies = dependencyMap.dependencies[file]

        let dependencySet: Set<string> = new Set<string>();
        fileDependencies.forEach((dependency) => {
            if (!fileToCycleMap.has(dependency)) {
                dependencySet.add(this.getJobId(dependency));
                return;
            }
            const dependencyCycle: string = fileToCycleMap.get(dependency)!

            if (fileToCycleMap.has(file)) {
                const fileCycle: string = fileToCycleMap.get(file)!
                if (fileCycle == dependencyCycle) {
                    return;
                }
            }
            dependencySet.add(dependencyCycle);
        });
        return dependencySet;
    }

    private collectJobDependants(
        file: string,
        dependencyMap: DependencyFileMap,
        fileToCycleMap: Map<string, string>
    ): Set<string> {
        const fileDependants = dependencyMap.dependants[file]
        let dependantSet: Set<string> = new Set<string>();

        fileDependants.forEach((dependant) => {
            if (!fileToCycleMap.has(dependant)) {
                dependantSet.add(this.getJobId(dependant));
                return;
            }
            const dependantCycle: string = fileToCycleMap.get(dependant)!

            if (fileToCycleMap.has(file)) {
                const fileCycle: string = fileToCycleMap.get(file)!
                if (fileCycle == dependantCycle) {
                    return;
                }
            }
            dependantSet.add(dependantCycle)
        });
        return dependantSet;
    }

    private getJobId(file: string): string {
        return computeHash(file);
    }
}
