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

import { Ets2panda } from '../util/ets2panda'
import {
    ARKTSCONFIG_JSON_FILE,
    LANGUAGE_VERSION,
    LINKER_INPUT_FILE,
    MERGED_ABC_FILE,
    CLUSTER_FILES_TRESHOLD,
    ENABLE_CLUSTERS,
    DEFAULT_WORKER_NUMS
} from '../pre_define';
import {
    ensurePathExists,
    isMac,
    checkDependencyModuleInfoCorrectness,
} from '../util/utils';
import {
    Logger,
    LogDataFactory,
} from '../logger';
import { DependencyAnalyzer } from '../dependency_analyzer';
import { ErrorCode, DriverError } from '../util/error';
import {
    BuildConfig,
    BUILD_MODE,
    DependencyModuleConfig,
    ModuleInfo,
    ProcessCompileTask,
    ProcessDeclgenV1Task,
    CompileJobInfo,
    CompileJobType,
    DeclgenV1JobInfo,
    ES2PANDA_MODE,
    OHOS_MODULE_TYPE
} from '../types';
import {
    ArkTSConfigGenerator
} from './generate_arktsconfig';
import {
    BS_PERF_FILE_NAME,
    StatisticsRecorder,
} from '../util/statsRecorder';
import {
    handleCompileProcessWorkerExit,
    handleDeclgenWorkerExit
} from '../util/worker_exit_handler';
import {
    DriverProcessFactory,
    TaskManager,
} from '../util/TaskManager';
import { Graph, GraphNode } from '../util/graph';

enum BuildSystemEvent {
    COLLECT_MODULES = 'Collect module infos',
    GEN_CONFIGS = 'Generate arktsconfigs for modules',
    PROCESS_ENTRY_FILES = 'Process entry files',
    BACKWARD_COMPAT = 'Backward compatibility stuff',
    DEPENDENCY_ANALYZER = 'Dependency analyzer',
    RUN_SIMULTANEOUS = 'Run simultaneous',
    RUN_PARALLEL = 'Run parallel',
    RUN_SEQUENTIAL = 'Run sequential',
    RUN_LINKER = 'Run linker',
    DECLGEN_V1_SEQUENTIAL = 'Generate v1 declaration files (sequential)',
    DECLGEN_V1_PARALLEL = 'Generate v1 declaration files (parallel)',
}

function formEvent(event: BuildSystemEvent): string {
    return '[Build system] ' + event;
}

export abstract class BaseMode {
    private buildConfig: BuildConfig;
    private entryFiles: Set<string>;
    private fileToModule: Map<string, ModuleInfo>;
    private moduleInfos: Map<string, ModuleInfo>;
    protected mergedAbcFile: string;
    protected logger: Logger;
    protected readonly statsRecorder: StatisticsRecorder;
    private readonly moduleType: OHOS_MODULE_TYPE;

    constructor(buildConfig: BuildConfig) {
        this.buildConfig = buildConfig;
        this.entryFiles = new Set<string>(buildConfig.compileFiles);
        this.fileToModule = new Map<string, ModuleInfo>();
        this.moduleInfos = new Map<string, ModuleInfo>();
        this.mergedAbcFile = path.resolve(this.outputDir, MERGED_ABC_FILE);
        this.logger = Logger.getInstance();
        this.moduleType = buildConfig.moduleType;

        this.statsRecorder = new StatisticsRecorder(
            path.resolve(this.cacheDir, BS_PERF_FILE_NAME),
            this.recordType,
            `Build system with mode: ${this.es2pandaMode}`
        );

        this.processBuildConfig();
        this.statsRecorder.record(formEvent(BuildSystemEvent.BACKWARD_COMPAT));
        this.backwardCompatibilityWorkaroundStub()
    }

    public get abcLinkerPath(): string | undefined {
        return this.buildConfig.abcLinkerPath
    }

    public get hasMainModule(): boolean {
        return this.buildConfig.hasMainModule
    }

    public get useEmptyPackage(): boolean {
        return this.buildConfig.useEmptyPackage ?? false
    }

    public get frameworkMode(): boolean {
        return this.buildConfig.frameworkMode ?? false
    }

    public get genDeclAnnotations(): boolean {
        return this.buildConfig.genDeclAnnotations ?? true
    }

    public get skipDeclCheck(): boolean {
        return this.buildConfig.skipDeclCheck ?? true;
    }

    public get es2pandaMode(): ES2PANDA_MODE {
        return this.buildConfig.es2pandaMode
    }

    public get dumpDependencyGraph(): boolean | undefined {
        return this.buildConfig.dumpDependencyGraph
    }

    public get entryFile(): string {
        return this.buildConfig.entryFile;
    }

    public get mainPackageName(): string {
        return this.buildConfig.packageName;
    }

    public get mainModuleRootPath(): string {
        return this.buildConfig.moduleRootPath;
    }

    public get mainModuleType(): string {
        return this.buildConfig.moduleType;
    }

    public get outputDir(): string {
        return this.buildConfig.loaderOutPath;
    }

    public get cacheDir(): string {
        return this.buildConfig.cachePath;
    }

    public get dependencyModuleList(): DependencyModuleConfig[] {
        return this.buildConfig.dependencyModuleList;
    }

    public get enableDeclgenEts2Ts(): boolean {
        return this.buildConfig.enableDeclgenEts2Ts;
    }

    public get isBuildConfigModified(): boolean | undefined {
        return this.buildConfig.isBuildConfigModified;
    }

    public set isBuildConfigModified(modified: boolean) {
        this.buildConfig.isBuildConfigModified = modified;
    }

    public get byteCodeHar(): boolean | undefined {
        return this.buildConfig.byteCodeHar;
    }

    public get mainSourceRoots(): string[] {
        return this.buildConfig.sourceRoots;
    }

    public get declgenV1OutPath(): string | undefined {
        return this.buildConfig.declgenV1OutPath;
    }

    public get declgenV2OutPath(): string | undefined {
        return this.buildConfig.declgenV2OutPath;
    }

    public get declgenBridgeCodePath(): string | undefined {
        return this.buildConfig.declgenBridgeCodePath;
    }

    public get isDebug(): boolean {
        return this.buildConfig.buildMode === BUILD_MODE.DEBUG;
    }

    public get dumpPerf(): boolean | undefined {
        return this.buildConfig.dumpPerf;
    }

    public get recordType(): 'OFF' | 'ON' | undefined {
        return this.buildConfig.recordType
    }

    private compile(id: string, job: CompileJobInfo): boolean {
        job.type = this.moduleType === OHOS_MODULE_TYPE.HAR ? CompileJobType.DECL_ABC : job.type;
        const ets2panda = Ets2panda.getInstance();
        let errOccurred = false;
        ets2panda.initalize();
        try {
            ets2panda.compile(id, job, this.isDebug)
        } catch (error) {
            if (error instanceof DriverError) {
                this.logger.printError(error.logData);
                errOccurred = true;
            }
        } finally {
            ets2panda.finalize()
        }

        return !errOccurred;
    }

    public async run(): Promise<void> {
        this.statsRecorder.record(formEvent(BuildSystemEvent.DEPENDENCY_ANALYZER));
        const depAnalyzer = new DependencyAnalyzer(this.buildConfig);
        const allOutputs: string[] = [];
        const buildGraph = depAnalyzer.getGraph(this.entryFiles, this.fileToModule, this.moduleInfos, allOutputs);
        if (!buildGraph.hasNodes()) {
            this.logger.printWarn('Nothing to compile. Exiting...')
            return;
        }

        this.statsRecorder.record(formEvent(BuildSystemEvent.RUN_SEQUENTIAL));

        // Just to init
        Ets2panda.getInstance(this.buildConfig)

        let success: boolean = true;
        const tasks: { id: string, job: CompileJobInfo }[] = Graph.topologicalSort(buildGraph)
            .map((nodeId) => { return buildGraph.getNodeById(nodeId); })
            .map((node) => { return { id: node.id, job: node.data }; })

        while (tasks.length > 0) {
            const task = tasks.shift()!;
            const job = task.job;
            const id = task.id;
            if (job.fileList.length > 1) {
                // Compile cycle simultaneous
                this.logger.printDebug('Compiling cycle....')
                this.logger.printDebug(`file list: \n${job.fileList.join('\n')}`)
                const res = this.compileSimultaneous(id, job)
                success = res && success;
            } else {
                const res = this.compile(id, job)
                success = res && success;
            }
        }

        Ets2panda.destroyInstance();

        if (!success) {
            const logData = LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_ERRORS_OCCURRED,
                'One or more errors occured.'
            );
            this.logger.printError(logData);
            throw new Error('Run failed.');
        }

        this.statsRecorder.record(formEvent(BuildSystemEvent.RUN_LINKER));
        this.mergeAbcFiles(allOutputs);
    }

    private compileSimultaneous(id: string, job: CompileJobInfo): boolean {
        job.type = this.moduleType === OHOS_MODULE_TYPE.HAR ? CompileJobType.DECL_ABC : job.type;
        const ets2panda = Ets2panda.getInstance(this.buildConfig);
        ets2panda.initalize();
        let errOccurred = false;
        try {
            ets2panda.compileSimultaneous(id, job, this.isDebug, this.dumpPerf)
        } catch (error) {
            if (error instanceof DriverError) {
                this.logger.printError(error.logData);
                errOccurred = true;
            }
        } finally {
            ets2panda.finalize()
        }

        return !errOccurred;
    }

    private mergeAbcFiles(abcFiles: string[]): void {
        let linkerInputFile: string = path.join(this.cacheDir, LINKER_INPUT_FILE);
        let linkerInputContent: string = abcFiles.join(os.EOL);

        fs.writeFileSync(linkerInputFile, linkerInputContent);
        let abcLinkerCmd = ['"' + this.abcLinkerPath + '"']
        abcLinkerCmd.push('--strip-unused');
        abcLinkerCmd.push('--output');
        abcLinkerCmd.push('"' + this.mergedAbcFile + '"');
        abcLinkerCmd.push('--');
        abcLinkerCmd.push('@' + '"' + linkerInputFile + '"');

        let abcLinkerCmdStr: string = abcLinkerCmd.join(' ');
        if (isMac()) {
            const loadLibrary = 'DYLD_LIBRARY_PATH=' + '"' + process.env.DYLD_LIBRARY_PATH + '"';
            abcLinkerCmdStr = loadLibrary + ' ' + abcLinkerCmdStr;
        }
        this.logger.printDebug(abcLinkerCmdStr);
        ensurePathExists(this.mergedAbcFile);
        try {
            child_process.execSync(abcLinkerCmdStr).toString();
        } catch (error) {
            if (error instanceof Error) {
                const logData =
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_LINK_ABC_FAIL,
                        'Link abc files failed.',
                        error.message
                    );
                this.logger.printError(logData);
            }
        }
    }

    private getDependencyModules(moduleInfo: ModuleInfo): Map<string, ModuleInfo>[] {
        const dynamicDependencyModules: Map<string, ModuleInfo> = new Map<string, ModuleInfo>();
        const staticDependencyModules: Map<string, ModuleInfo> = new Map<string, ModuleInfo>();

        // NOTE: workaround
        // NOTE: to be refactored
        this.processDependencyModule(moduleInfo.packageName, moduleInfo, dynamicDependencyModules, staticDependencyModules)

        if (moduleInfo.dependencies) {
            moduleInfo.dependencies.forEach((packageName: string) => {
                let dependency: ModuleInfo | undefined = this.moduleInfos.get(packageName);
                if (!dependency) {
                    throw new DriverError(
                        LogDataFactory.newInstance(
                            ErrorCode.BUILDSYSTEM_DEPENDENT_MODULE_INFO_NOT_FOUND,
                            `Module ${packageName} is not found in dependencyModuleList`
                        )
                    );
                }
                this.processDependencyModule(packageName, dependency, dynamicDependencyModules, staticDependencyModules);
            });
        }
        return [dynamicDependencyModules, staticDependencyModules];
    }

    private processDependencyModule(
        packageName: string,
        module: ModuleInfo,
        dynamicDependencyModules: Map<string, ModuleInfo>,
        staticDependencyModules: Map<string, ModuleInfo>
    ): void {
        if (module.language === LANGUAGE_VERSION.ARKTS_1_2) {
            staticDependencyModules.set(packageName, module);
        } else if (module.language === LANGUAGE_VERSION.ARKTS_1_1) {
            dynamicDependencyModules.set(packageName, module);
        } else if (module.language === LANGUAGE_VERSION.ARKTS_HYBRID) {
            staticDependencyModules.set(packageName, module);
            dynamicDependencyModules.set(packageName, module);
        }
    }

    protected generateArkTSConfigForModules(): void {
        // Just to init the generator
        ArkTSConfigGenerator.getInstance(this.buildConfig)

        const dependenciesSets = new Map<string, Set<ModuleInfo>>;

        // Fill dependenciesSets and generate ArktsConfigs
        this.moduleInfos.forEach((moduleInfo: ModuleInfo) => {
            dependenciesSets.set(moduleInfo.packageName, new Set())
            moduleInfo.dependencies?.forEach((dependency: string) => {
                dependenciesSets.get(moduleInfo.packageName)!.add(this.moduleInfos.get(dependency)!)
            });

            ArkTSConfigGenerator.getInstance().generateArkTSConfigFile(moduleInfo, this.enableDeclgenEts2Ts);
        });

        // Merge ArktsConfigs
        dependenciesSets.forEach((dependencies: Set<ModuleInfo>, module: string) => {
            let moduleInfo = this.moduleInfos.get(module)!
            let arktsConfig = ArkTSConfigGenerator.getInstance().getArktsConfigByPackageName(module)!;
            dependencies.forEach((dependency: ModuleInfo) => {
                arktsConfig.mergeArktsConfig(
                    ArkTSConfigGenerator.getInstance().getArktsConfigByPackageName(dependency.packageName)!
                )

            });
            fs.writeFileSync(moduleInfo.arktsConfigFile, JSON.stringify(arktsConfig.object, null, 2))
        });
    }

    private collectModuleDependencies(): void {
        this.moduleInfos.forEach((moduleInfo: ModuleInfo) => {
            let [dynamicDepModules, staticDepModules] = this.getDependencyModules(moduleInfo);
            moduleInfo.dynamicDependencyModules = dynamicDepModules;
            moduleInfo.staticDependencyModules = staticDepModules;
        });
    }

    protected collectModuleInfos(): void {
        // NOTE: workaround for frameworkMode
        if (this.hasMainModule && (!this.mainPackageName || !this.mainModuleRootPath || !this.mainSourceRoots)) {
            throw new DriverError(
                LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_MODULE_INFO_NOT_CORRECT_FAIL,
                    'Main module info is not correct.'
                )
            );
        }

        const mainModuleInfo: ModuleInfo = this.getMainModuleInfo();
        this.moduleInfos.set(this.mainPackageName, mainModuleInfo);

        this.dependencyModuleList.forEach((dependency: DependencyModuleConfig) => {
            if (!checkDependencyModuleInfoCorrectness(dependency)) {
                throw new DriverError(
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_DEPENDENT_MODULE_INFO_NOT_CORRECT_FAIL,
                        'Dependent module info is not correct.'
                    )
                );
            }
            // workaround: information for main module is filled incorrectly
            if (dependency.packageName === mainModuleInfo.packageName) {
                mainModuleInfo.declFilesPath = dependency.declFilesPath;
                mainModuleInfo.language = dependency.language;
            }
            if (this.moduleInfos.has(dependency.packageName)) {
                return;
            }
            // NOTE: workaround
            // NOTE: to be refactored
            const getNormalizedEntryFile = (dependency: DependencyModuleConfig): string => {
                if (path.isAbsolute(dependency.entryFile)) {
                    return path.relative(dependency.modulePath, dependency.entryFile)
                }
                return dependency.entryFile
            }

            let moduleInfo: ModuleInfo = {
                isMainModule: false,
                packageName: dependency.packageName,
                moduleRootPath: dependency.modulePath,
                moduleType: dependency.moduleType,
                sourceRoots: dependency.sourceRoots,
                entryFile: getNormalizedEntryFile(dependency),
                arktsConfigFile: path.resolve(this.cacheDir, dependency.packageName, ARKTSCONFIG_JSON_FILE),
                dynamicDependencyModules: new Map<string, ModuleInfo>(),
                staticDependencyModules: new Map<string, ModuleInfo>(),
                declgenV1OutPath: dependency.declgenV1OutPath,
                declgenV2OutPath: dependency.declgenV2OutPath,
                declgenBridgeCodePath: dependency.declgenBridgeCodePath,
                language: dependency.language,
                declFilesPath: dependency.declFilesPath,
                dependencies: dependency.dependencies ?? [],
                byteCodeHar: dependency.byteCodeHar,
                abcPath: dependency.abcPath,
            };
            this.moduleInfos.set(dependency.packageName, moduleInfo);
            this.moduleInfos.get(this.mainPackageName)!.dependencies.push(dependency.packageName)
        });

        this.collectModuleDependencies();
    }

    protected getMainModuleInfo(): ModuleInfo {
        return {
            isMainModule: true,
            packageName: this.mainPackageName,
            moduleRootPath: this.mainModuleRootPath,
            moduleType: this.mainModuleType,
            sourceRoots: this.mainSourceRoots,
            // NOTE: workaround. (entryFile is almost always undefined)
            // NOTE: to be refactored
            entryFile: this.entryFile ?? '',
            arktsConfigFile: path.resolve(this.cacheDir, this.mainPackageName, ARKTSCONFIG_JSON_FILE),
            dynamicDependencyModules: new Map<string, ModuleInfo>(),
            staticDependencyModules: new Map<string, ModuleInfo>(),
            declgenV1OutPath: this.declgenV1OutPath,
            declgenV2OutPath: this.declgenV2OutPath,
            declgenBridgeCodePath: this.declgenBridgeCodePath,
            byteCodeHar: this.byteCodeHar,
            // workaround! Should be fixed
            language: LANGUAGE_VERSION.ARKTS_1_2,
            dependencies: []
        };
    }

    protected processEntryFiles(): void {
        this.entryFiles.forEach((file: string) => {
            for (const [_, moduleInfo] of this.moduleInfos) {
                const relativePath = path.relative(moduleInfo.moduleRootPath, file);
                if (relativePath.startsWith('..') || path.isAbsolute(relativePath)) {
                    continue;
                }
                this.fileToModule.set(path.resolve(file), moduleInfo);
                return;
            }
            throw new DriverError(
                LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_FILE_NOT_BELONG_TO_ANY_MODULE_FAIL,
                    'File does not belong to any module in moduleInfos.',
                    '',
                    file
                )
            );
        });
        this.logger.printDebug(`collected fileToModule ${JSON.stringify([...this.fileToModule.entries()], null, 1)}`)
    }

    protected processBuildConfig(): void {
        this.statsRecorder.record(formEvent(BuildSystemEvent.COLLECT_MODULES));
        this.collectModuleInfos();
        this.logger.printDebug(`ModuleInfos: ${JSON.stringify([...this.moduleInfos], null, 1)}`)
        this.statsRecorder.record(formEvent(BuildSystemEvent.GEN_CONFIGS));
        this.generateArkTSConfigForModules();
        this.statsRecorder.record(formEvent(BuildSystemEvent.PROCESS_ENTRY_FILES));
        this.processEntryFiles();
    }

    protected backwardCompatibilityWorkaroundStub(): void {
        const mainModule: ModuleInfo = this.moduleInfos.get(this.mainPackageName)!
        // NOTE: workaround (just to add entryFile to mainModule)
        // NOTE: to be refactored
        const mainModuleFileList: string[] = [...this.fileToModule.entries()].filter(([_, module]: [string, ModuleInfo]) => {
            return module.isMainModule
        }).map(([file, _]: [string, ModuleInfo]) => { return file })
        mainModule.entryFile = mainModuleFileList[0]

        this.logger.printDebug(`mainModule entryFile: ${mainModule.entryFile}`)
    }

    public async runSimultaneous(): Promise<void> {
        this.statsRecorder.record(formEvent(BuildSystemEvent.RUN_SIMULTANEOUS));

        const mainModule: ModuleInfo | undefined = this.moduleInfos.get(this.mainPackageName)

        // NOTE: workaround (main module entry file problem)
        // NOTE: to be refactored
        let entryFile: string;
        let module: ModuleInfo;
        if (!mainModule || !mainModule.entryFile) {
            entryFile = [...this.entryFiles][0]
            module = this.fileToModule.get(entryFile)!
        } else {
            entryFile = mainModule.entryFile
            module = mainModule
        }

        let outputFile: string = this.mergedAbcFile
        let arktsConfigFile: string = module.arktsConfigFile;

        this.logger.printDebug(`entryFile: ${entryFile}`)
        this.logger.printDebug(`module: ${JSON.stringify(module, null, 1)}`)
        this.logger.printDebug(`arktsConfigFile: ${arktsConfigFile}`)

        // Just to init
        Ets2panda.getInstance(this.buildConfig)
        // We do not need any queues just compile a bunch of files
        // Ets2panda will build it simultaneous
        let res = this.compileSimultaneous('SimultaneousBuildId', {
            fileList: [...this.entryFiles],
            fileInfo: {
                input: entryFile,
                output: outputFile,
                arktsConfig: arktsConfigFile,
                moduleName: module.packageName,
                moduleRoot: module.moduleRootPath
            },
            declgenConfig: {
                output: module.declgenV2OutPath!
            },
            type: CompileJobType.ABC
        });
        Ets2panda.destroyInstance();
        if (!res) {
            throw new Error('Simultaneous build failed.');
        }
    }

    public async runParallel(): Promise<void> {
        if (ENABLE_CLUSTERS && this.entryFiles.size <= CLUSTER_FILES_TRESHOLD) {
            await this.runSimultaneous();
            return;
        }
        this.statsRecorder.record(formEvent(BuildSystemEvent.DEPENDENCY_ANALYZER));
        const depAnalyzer = new DependencyAnalyzer(this.buildConfig);
        const allOutputs: string[] = [];
        const buildGraph = depAnalyzer.getGraph(this.entryFiles, this.fileToModule, this.moduleInfos, allOutputs);
        if (!buildGraph.hasNodes()) {
            this.logger.printWarn('Nothing to compile. Exiting...')
            return;
        }

        this.statsRecorder.record(formEvent(BuildSystemEvent.RUN_PARALLEL));

        const taskManager = new TaskManager<ProcessCompileTask>(handleCompileProcessWorkerExit, false, DEFAULT_WORKER_NUMS);
        const workerFactory = new DriverProcessFactory(
            path.resolve(__dirname, 'compile_process_worker.js'),
            ['process child:' + __filename],
            {
                stdio: ['inherit', 'inherit', 'inherit', 'ipc']
            }
        );
        taskManager.startWorkers(workerFactory);

        const newNodes: GraphNode<ProcessCompileTask>[] = [];
        for (const node of buildGraph.nodes) {
            const newType = this.moduleType === OHOS_MODULE_TYPE.HAR ? CompileJobType.DECL_ABC : node.data.type;
            newNodes.push({
                id: node.id,
                data: {
                    ...node.data,
                    type: newType,
                    buildConfig: this.buildConfig
                },
                predecessors: node.predecessors,
                descendants: node.descendants,
            })
        }
        const newGraph: Graph<ProcessCompileTask> = Graph.createGraphFromNodes(newNodes);

        taskManager.buildGraph = newGraph;
        taskManager.initTaskQueue();
        const res = await taskManager.finish();

        if (!res) {
            const logData = LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_ERRORS_OCCURRED,
                'One or more errors occured.'
            );
            this.logger.printError(logData);
            throw new Error('Parallel run failed.');
        }

        this.statsRecorder.record(formEvent(BuildSystemEvent.RUN_LINKER));
        this.mergeAbcFiles(allOutputs);
    }

    public async runConcurrent(): Promise<void> {
        // NOTE: TBD
        throw new DriverError(
            LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL,
                'Concurrent mode is currently unavailable.'
            )
        )
    }

    private declgenV1(job: CompileJobInfo): boolean {
        let module = this.fileToModule.get(job.fileInfo.input)!;
        let declgenV1OutPath: string = module.declgenV1OutPath!;
        let declgenBridgeCodePath: string = module.declgenBridgeCodePath!;
        let declgenJob: DeclgenV1JobInfo = { ...job, declgenConfig: { output: declgenV1OutPath, bridgeCode: declgenBridgeCodePath } }

        let result = true;
        const ets2panda = Ets2panda.getInstance();
        ets2panda.initalize();
        try {
            ets2panda.declgenV1(declgenJob, this.skipDeclCheck, this.genDeclAnnotations)
        } catch (error) {
            // Report the error, do not crash the declgen process
            const err = error as DriverError
            this.logger.printError(err.logData)
            result = false;
        } finally {
            ets2panda.finalize()
        }
        return result;
    }


    public async generateDeclarationV1(): Promise<void> {
        this.statsRecorder.record(formEvent(BuildSystemEvent.DEPENDENCY_ANALYZER));
        const depAnalyzer = new DependencyAnalyzer(this.buildConfig, false);
        const buildGraph = depAnalyzer.getGraph(this.entryFiles, this.fileToModule, this.moduleInfos, []);
        if (!buildGraph.hasNodes()) {
            this.logger.printWarn('Nothing to compile. Exiting...')
            return;
        }

        this.statsRecorder.record(formEvent(BuildSystemEvent.DECLGEN_V1_SEQUENTIAL));

        const jobs: CompileJobInfo[] = Graph.topologicalSort(buildGraph)
            .map((nodeId: string) => { return buildGraph.getNodeById(nodeId); })
            .map((node) => { return node.data; });

        // Just to init
        Ets2panda.getInstance(this.buildConfig)

        let success: boolean = true;
        while (jobs.length > 0) {
            let job: CompileJobInfo = jobs.shift()!;
            const res: boolean = this.declgenV1(job)
            success = res && success;
        }

        Ets2panda.destroyInstance();

        if (!success) {
            throw new DriverError(
                LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_ERRORS_OCCURRED,
                    'One or more errors occured.'
                )
            );
        }
    }

    public async generateDeclarationV1Parallel(): Promise<void> {
        this.statsRecorder.record(formEvent(BuildSystemEvent.DEPENDENCY_ANALYZER));
        const depAnalyzer = new DependencyAnalyzer(this.buildConfig, false);
        const buildGraph = depAnalyzer.getGraph(this.entryFiles, this.fileToModule, this.moduleInfos, []);
        if (!buildGraph.hasNodes()) {
            this.logger.printWarn('Nothing to compile. Exiting...')
            return;
        }

        this.statsRecorder.record(formEvent(BuildSystemEvent.DECLGEN_V1_PARALLEL));

        const taskManager = new TaskManager<ProcessDeclgenV1Task>(handleDeclgenWorkerExit, true);
        const workerFactory = new DriverProcessFactory(
            path.resolve(__dirname, 'declgen_process_worker.js'),
            ['process child:' + __filename],
            {
                stdio: ['inherit', 'inherit', 'inherit', 'ipc']
            }
        );
        taskManager.startWorkers(workerFactory);

        const newNodes: GraphNode<ProcessDeclgenV1Task>[] = [];
        for (const node of buildGraph.nodes) {
            const module = this.fileToModule.get(node.data.fileList[0]!)!
            const declgenV1OutPath: string = module.declgenV1OutPath!
            const declgenBridgeCodePath: string = module.declgenBridgeCodePath!
            newNodes.push({
                id: node.id,
                data: {
                    ...node.data,
                    declgenConfig: {
                        output: declgenV1OutPath,
                        bridgeCode: declgenBridgeCodePath
                    },
                    buildConfig: this.buildConfig
                },
                predecessors: node.predecessors,
                descendants: node.descendants,
            })
        }
        const newGraph: Graph<ProcessDeclgenV1Task> = Graph.createGraphFromNodes(newNodes);

        taskManager.buildGraph = newGraph;
        taskManager.initTaskQueue();
        // Ignore the result
        await taskManager.finish();
    }
}
