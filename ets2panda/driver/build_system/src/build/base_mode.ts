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
    ABC_SUFFIX,
    ARKTSCONFIG_JSON_FILE,
    LANGUAGE_VERSION,
    LINKER_INPUT_FILE,
    MERGED_ABC_FILE,
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
    JobInfo
} from '../types';
import {
    ArkTSConfigGenerator
} from './generate_arktsconfig';
import {
    BS_PERF_FILE_NAME,
    CompileSingleData,
    RECORDE_MODULE_NODE,
} from '../util/record_time_mem';
import {
    handleCompileProcessWorkerExit,
    handleDeclgenWorkerExit
} from '../util/worker_exit_handler';
import {
    DriverProcessFactory,
    TaskManager,
} from '../util/TaskManager';

import { dotGraphDump } from '../util/dotGraphDump'

export abstract class BaseMode {
    private buildConfig: BuildConfig;
    private entryFiles: Set<string>;
    private fileToModule: Map<string, ModuleInfo>;
    private moduleInfos: Map<string, ModuleInfo>;
    protected mergedAbcFile: string;
    protected logger: Logger;
    private jobs: Record<string, CompileJobInfo>;
    private jobQueue: CompileJobInfo[];
    private completedJobQueue: CompileJobInfo[];

    constructor(buildConfig: BuildConfig) {
        this.buildConfig = buildConfig;
        this.entryFiles = new Set<string>(buildConfig.compileFiles);
        this.fileToModule = new Map<string, ModuleInfo>();
        this.moduleInfos = new Map<string, ModuleInfo>();
        this.mergedAbcFile = path.resolve(this.outputDir, MERGED_ABC_FILE);
        this.logger = Logger.getInstance();
        this.jobs = {};
        this.jobQueue = [];
        this.completedJobQueue = [];

        this.processBuildConfig();
        this.backwardCompatibilityWorkaroundStub()
    }

    public get abcLinkerPath() {
        return this.buildConfig.abcLinkerPath
    }

    public get hasMainModule() {
        return this.buildConfig.hasMainModule
    }

    public get useEmptyPackage() {
        return this.buildConfig.useEmptyPackage ?? false
    }

    public get frameworkMode() {
        return this.buildConfig.frameworkMode ?? false
    }

    public get genDeclAnnotations() {
        return this.buildConfig.genDeclAnnotations ?? true
    }

    public get skipDeclCheck() {
        return this.buildConfig.skipDeclCheck ?? true;
    }

    public get es2pandaMode() {
        return this.buildConfig.es2pandaMode
    }

    public get dumpDependencyGraph() {
        return this.buildConfig.dumpDependencyGraph
    }

    public get entryFile() {
        return this.buildConfig.entryFile;
    }

    public get mainPackageName() {
        return this.buildConfig.packageName;
    }

    public get mainModuleRootPath() {
        return this.buildConfig.moduleRootPath;
    }

    public get mainModuleType() {
        return this.buildConfig.moduleType;
    }

    public get outputDir() {
        return this.buildConfig.loaderOutPath;
    }

    public get cacheDir() {
        return this.buildConfig.cachePath;
    }

    public get dependencyModuleList() {
        return this.buildConfig.dependencyModuleList;
    }

    public get enableDeclgenEts2Ts() {
        return this.buildConfig.enableDeclgenEts2Ts;
    }

    public get isBuildConfigModified(): boolean | undefined {
        return this.buildConfig.isBuildConfigModified;
    }

    public set isBuildConfigModified(modified: boolean) {
        this.buildConfig.isBuildConfigModified = modified;
    }

    public get byteCodeHar() {
        return this.buildConfig.byteCodeHar;
    }

    public get mainSourceRoots() {
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

    public get isDebug() {
        return this.buildConfig.buildMode === BUILD_MODE.DEBUG;
    }

    private compile(job: CompileJobInfo): boolean {
        const ets2panda = Ets2panda.getInstance();
        let errOccurred = false;
        ets2panda.initalize();
        try {
            ets2panda.compile(job, this.isDebug)
        } catch (error) {
            const err = error as DriverError
            this.logger.printError(err.logData);
            errOccurred = true;
        } finally {
            ets2panda.finalize()
        }

        return !errOccurred;
    }

    public async run(): Promise<void> {
        const depAnalyzer = new DependencyAnalyzer(this.buildConfig);
        this.jobs = depAnalyzer.collectJobs(this.entryFiles, this.fileToModule, this.moduleInfos);

        if (Object.entries(this.jobs).length == 0) {
            this.logger.printWarn("Nothing to compile. Exiting...")
            return;
        }

        if (this.dumpDependencyGraph) {
            fs.writeFileSync(path.resolve(this.cacheDir, 'graph.dot'), dotGraphDump(this.jobs, this.fileToModule), 'utf-8')
        }

        this.initCompileQueues();
        // Just to init
        Ets2panda.getInstance(this.buildConfig)

        let success: boolean = true;
        while (this.haveQueuedJobs()) {
            let job: CompileJobInfo = this.consumeJob()!
            if (job.fileList.length > 1) {
                // Compile cycle simultaneous
                this.logger.printDebug("Compiling cycle....")
                this.logger.printDebug(`file list: \n${job.fileList.join('\n')}`)
                const res = this.compileSimultaneous(job)
                success = res && success;
            } else {
                const res = this.compile(job)
                success = res && success;
            }
            this.dispatchNextJob(job)
        }
        if (!success) {
            throw new DriverError(
                LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_ERRORS_OCCURRED,
                    'One or more errors occured.'
                )
            );
        }

        this.mergeAbcFiles();
        Ets2panda.destroyInstance();

    }

    private compileSimultaneous(job: CompileJobInfo): boolean {
        const ets2panda = Ets2panda.getInstance(this.buildConfig);
        ets2panda.initalize();
        let errOccurred = false;
        try {
            ets2panda.compileSimultaneous(job, this.isDebug)
        } catch (error) {
            const err = error as DriverError
            this.logger.printError(err.logData);
            errOccurred = true;
        } finally {
            ets2panda.finalize()
        }

        return !errOccurred;
    }

    private mergeAbcFiles(): void {
        let linkerInputFile: string = path.join(this.cacheDir, LINKER_INPUT_FILE);
        let linkerInputContent: string = '';
        this.completedJobQueue.forEach((job: CompileJobInfo) => {
            if (job.fileInfo.output.endsWith(ABC_SUFFIX)) {
                linkerInputContent += job.fileInfo.output + os.EOL;
            }
        })

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
                throw new DriverError(
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_LINK_ABC_FAIL,
                        'Link abc files failed.',
                        error.message
                    )
                );
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
        let compileSingleData = new CompileSingleData(path.join(path.resolve(), BS_PERF_FILE_NAME));
        compileSingleData.record(RECORDE_MODULE_NODE.COLLECT_INFO);
        this.collectModuleInfos();
        this.logger.printDebug(`ModuleInfos: ${JSON.stringify([...this.moduleInfos], null, 1)}`)
        compileSingleData.record(RECORDE_MODULE_NODE.GEN_CONFIG, RECORDE_MODULE_NODE.COLLECT_INFO);
        this.generateArkTSConfigForModules();
        compileSingleData.record(RECORDE_MODULE_NODE.CLT_FILES, RECORDE_MODULE_NODE.GEN_CONFIG);
        this.processEntryFiles();
        compileSingleData.record(RECORDE_MODULE_NODE.END, RECORDE_MODULE_NODE.CLT_FILES);
        compileSingleData.writeSumSingle(path.resolve());
    }

    protected backwardCompatibilityWorkaroundStub() {
        const mainModule: ModuleInfo = this.moduleInfos.get(this.mainPackageName)!
        // NOTE: workaround (just to add entryFile to mainModule)
        // NOTE: to be refactored
        if (Object.keys(this.jobs).length == 0) {
            const mainModuleFileList: string[] = [...this.fileToModule.entries()].filter(([_, module]: [string, ModuleInfo]) => {
                return module.isMainModule
            }).map(([file, _]: [string, ModuleInfo]) => { return file })
            mainModule.entryFile = mainModuleFileList[0]
        } else {
            mainModule.entryFile = Object.entries(this.jobs).filter(([_, job]: [string, JobInfo]) => {
                return job.jobDependants.length == 0
            })[0][1].fileList[0]
        }
        this.logger.printDebug(`mainModule entryFile: ${mainModule.entryFile}`)
    }

    public async runSimultaneous(): Promise<void> {
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
        ensurePathExists(outputFile)

        let arktsConfigFile: string = module.arktsConfigFile;

        this.logger.printDebug(`entryFile: ${entryFile}`)
        this.logger.printDebug(`module: ${JSON.stringify(module, null, 1)}`)
        this.logger.printDebug(`arktsConfigFile: ${arktsConfigFile}`)

        // Just to init
        Ets2panda.getInstance(this.buildConfig)
        // We do not need any queues just compile a bunch of files
        // Ets2panda will build it simultaneous
        this.compileSimultaneous({
            id: outputFile,
            fileList: [...this.entryFiles],
            jobDependencies: [],
            jobDependants: [],
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
        Ets2panda.destroyInstance()
    }

    public async runParallel(): Promise<void> {
        const depAnalyzer = new DependencyAnalyzer(this.buildConfig);
        this.jobs = depAnalyzer.collectJobs(this.entryFiles, this.fileToModule, this.moduleInfos);

        if (Object.entries(this.jobs).length == 0) {
            this.logger.printWarn("Nothing to compile. Exiting...")
            return;
        }

        if (this.dumpDependencyGraph) {
            fs.writeFileSync(path.resolve(this.cacheDir, 'graph.dot'), dotGraphDump(this.jobs, this.fileToModule), 'utf-8')
        }

        const taskManager = new TaskManager<ProcessCompileTask>(handleCompileProcessWorkerExit);
        const workerFactory = new DriverProcessFactory(
            path.resolve(__dirname, 'compile_process_worker.js'),
            ['process child:' + __filename],
            {
                stdio: ['inherit', 'inherit', 'inherit', 'ipc']
            }
        );
        taskManager.startWorkers(workerFactory);

        for (const job of Object.values(this.jobs)) {
            taskManager.submitTask(job.id, { ...job, buildConfig: this.buildConfig });
        }
        taskManager.initTaskQueue();
        const res = await taskManager.finish();

        if (!res) {
            throw new DriverError(
                LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_ERRORS_OCCURRED,
                    'One or more errors occured.'
                )
            );
        }

        this.completedJobQueue = Object.values(this.jobs)
        this.mergeAbcFiles()
    }

    private addJobToQueue(job: CompileJobInfo): void {
        if (this.jobQueue.some((queuedJob: JobInfo) => queuedJob.id === job.id)) {
            this.logger.printWarn(`Detected job duplication: job.id == ${job.id}`)
            return;
        }

        this.logger.printDebug(`Added Job ${JSON.stringify(job, null, 1)} to the queue`)
        this.jobQueue.push(job);
    }

    private initCompileQueues(): void {
        Object.values(this.jobs).forEach((job: CompileJobInfo) => {
            if (job.jobDependencies.length === 0) {
                this.addJobToQueue(job);
            }
        });
    }

    private dispatchNextJob(completedJob: CompileJobInfo) {
        let completedJobId = completedJob.id;
        this.logger.printDebug(`Removed Job ${completedJobId} from the queue`)
        completedJob.jobDependants.forEach((dependantJobId: string) => {
            const depJob: CompileJobInfo = this.jobs[dependantJobId];
            const depIndex = depJob.jobDependencies.indexOf(completedJobId);
            if (depIndex !== -1) {
                depJob.jobDependencies.splice(depIndex, 1);
                if (depJob.jobDependencies.length === 0) {
                    this.addJobToQueue(depJob);
                } else {
                    this.logger.printDebug(`Job ${depJob.id} still have dependencies ${JSON.stringify(depJob.jobDependencies, null, 1)}`)
                }
            }
        });
        this.completedJobQueue.push(completedJob)
    }

    private haveQueuedJobs(): boolean {
        return (this.jobQueue.length > 0);
    }

    private consumeJob(): CompileJobInfo | null {
        if (this.jobQueue.length == 0) {
            this.logger.printDebug("Job queue is empty!")
            return null;
        }

        return this.jobQueue.shift()!
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

    private declgenV1(job: CompileJobInfo): void {
        let module = this.fileToModule.get(job.fileInfo.input)!;
        let declgenV1OutPath: string = module.declgenV1OutPath!;
        let declgenBridgeCodePath: string = module.declgenBridgeCodePath!;
        let declgenJob: DeclgenV1JobInfo = { ...job, declgenConfig: { otuput: declgenV1OutPath, bridgeCode: declgenBridgeCodePath } }

        const ets2panda = Ets2panda.getInstance();
        ets2panda.initalize();
        try {
            ets2panda.declgenV1(declgenJob, this.skipDeclCheck, this.genDeclAnnotations)
        } catch (error) {
            // Report the error, do not crash the declgen process
            const err = error as DriverError
            this.logger.printError(err.logData)
        } finally {
            ets2panda.finalize()
        }
    }


    public async generateDeclarationV1(): Promise<void> {
        const depAnalyzer = new DependencyAnalyzer(this.buildConfig);
        this.jobs = depAnalyzer.collectJobs(this.entryFiles, this.fileToModule, this.moduleInfos);

        if (Object.entries(this.jobs).length == 0) {
            this.logger.printWarn("Nothing to compile. Exiting...")
            return;
        }

        if (this.dumpDependencyGraph) {
            fs.writeFileSync(path.resolve(this.cacheDir, 'graph.dot'), dotGraphDump(this.jobs, this.fileToModule), 'utf-8')
        }

        this.initCompileQueues();

        while (this.haveQueuedJobs()) {
            let job: CompileJobInfo = this.consumeJob()!
            this.declgenV1(job)
            this.dispatchNextJob(job)
        }
    }

    public async generateDeclarationV1Parallel(): Promise<void> {
        const depAnalyzer = new DependencyAnalyzer(this.buildConfig);
        this.jobs = depAnalyzer.collectJobs(this.entryFiles, this.fileToModule, this.moduleInfos);

        if (Object.entries(this.jobs).length == 0) {
            this.logger.printWarn("Nothing to compile. Exiting...")
            return;
        }

        if (this.dumpDependencyGraph) {
            fs.writeFileSync(path.resolve(this.cacheDir, 'graph.dot'), dotGraphDump(this.jobs, this.fileToModule), 'utf-8')
        }

        const taskManager = new TaskManager<ProcessDeclgenV1Task>(handleDeclgenWorkerExit, true);
        const workerFactory = new DriverProcessFactory(
            path.resolve(__dirname, 'declgen_process_worker.js'),
            ['process child:' + __filename],
            {
                stdio: ['inherit', 'inherit', 'inherit', 'ipc']
            }
        );
        taskManager.startWorkers(workerFactory);

        for (const job of Object.values(this.jobs)) {
            const module = this.fileToModule.get(job.fileList[0]!)!
            const declgenV1OutPath: string = module.declgenV1OutPath!
            const declgenBridgeCodePath: string = module.declgenBridgeCodePath!
            taskManager.submitTask(job.id, {
                ...job,
                declgenConfig: {
                    otuput: declgenV1OutPath,
                    bridgeCode: declgenBridgeCodePath
                },
                buildConfig: this.buildConfig
            });
        }
        taskManager.initTaskQueue();
        // Ignore the result
        await taskManager.finish();
    }
}
