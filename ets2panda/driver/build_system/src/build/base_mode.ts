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
import { initKoalaModules } from '../init/init_koala_modules';

import {
    ABC_SUFFIX,
    ARKTSCONFIG_JSON_FILE,
    DECL_ETS_SUFFIX,
    DECL_TS_SUFFIX,
    LANGUAGE_VERSION,
    LINKER_INPUT_FILE,
    MERGED_ABC_FILE,
    STATIC_RECORD_FILE,
    STATIC_RECORD_FILE_CONTENT,
    TS_SUFFIX
} from '../pre_define';
import {
    changeDeclgenFileExtension,
    changeFileExtension,
    createFileIfNotExists,
    ensurePathExists,
    isMac,
    isMixCompileProject,
    checkDependencyModuleInfoCorrectness,
    formEts2pandaCmd,
} from '../util/utils';
import {
    PluginDriver,
    PluginHook
} from '../plugins/plugins_driver';
import {
    Logger,
    LogDataFactory
} from '../logger';
import { DependencyAnalyzer } from '../dependency_analyzer';
import { ErrorCode, DriverError } from '../util/error';
import {
    BuildConfig,
    BUILD_MODE,
    CompileFileInfo,
    DependencyModuleConfig,
    ModuleInfo,
    ProcessCompileTask,
    CompileJobInfo,
    CompileJobType,
    JobInfo
} from '../types';
import {
    ArkTSConfigGenerator
} from './generate_arktsconfig';
import { KitImportTransformer } from '../plugins/KitImportTransformer';
import {
    BS_PERF_FILE_NAME,
    CompileSingleData,
    RECORDE_COMPILE_NODE,
    RECORDE_MODULE_NODE,
    RECORDE_RUN_NODE
} from '../util/record_time_mem';
import {
    handleCompileProcessWorkerExit,
    handleDeclgenWorkerExit
} from '../util/worker_exit_handler';
import {
    WorkerInfo,
    TaskManager,
    DriverProcess,
    DriverThread
} from '../util/TaskManager';

import { dotGraphDump } from '../util/dotGraphDump'

export abstract class BaseMode {
    private buildConfig: BuildConfig;
    public entryFiles: Set<string>;
    public fileToModule: Map<string, ModuleInfo>;
    public moduleInfos: Map<string, ModuleInfo>;
    public mergedAbcFile: string;
    public logger: Logger;
    public depAnalyzer: DependencyAnalyzer;
    public abcFiles: Set<string>;
    public jobs: Record<string, CompileJobInfo>;
    public jobQueue: CompileJobInfo[];
    public completedJobQueue: CompileJobInfo[];
    // NOTE: should be Ets2panda Wrapper Module
    // NOTE: to be refactored
    public koalaModule: any;

    constructor(buildConfig: BuildConfig) {
        this.buildConfig = buildConfig;
        this.entryFiles = new Set<string>(buildConfig.compileFiles);
        this.fileToModule = new Map<string, ModuleInfo>();
        this.moduleInfos = new Map<string, ModuleInfo>();
        this.mergedAbcFile = path.resolve(this.outputDir, MERGED_ABC_FILE);
        this.logger = Logger.getInstance();
        this.depAnalyzer = new DependencyAnalyzer(this.buildConfig);
        this.abcFiles = new Set<string>();
        this.jobs = {};
        this.jobQueue = [];
        this.completedJobQueue = [];
        this.koalaModule = initKoalaModules(buildConfig)

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

    public get es2pandaDepGraphDotDump() {
        return this.buildConfig.es2pandaDepGraphDotDump
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

    private compile(job: CompileJobInfo) {
        this.logger.printDebug("compile START")
        this.logger.printDebug(`job ${JSON.stringify(job, null, 1)}`)

        const { inputFilePath, outputFilePath }: CompileFileInfo = job.compileFileInfo;
        const outputDeclFilePath = changeFileExtension(outputFilePath, DECL_ETS_SUFFIX)
        ensurePathExists(outputDeclFilePath);

        const source = fs.readFileSync(inputFilePath, 'utf-8');

        const ets2pandaCmd: string[] = formEts2pandaCmd(job, this.isDebug)
        this.logger.printDebug('ets2pandaCmd: ' + ets2pandaCmd.join(' '));

        const { arkts, arktsGlobal } = this.koalaModule;

        try {
            arktsGlobal.filePath = inputFilePath;
            arktsGlobal.config = arkts.Config.create(ets2pandaCmd).peer;
            arktsGlobal.compilerContext = arkts.Context.createFromString(source);
            PluginDriver.getInstance().getPluginContext().setArkTSProgram(arktsGlobal.compilerContext.program);

            arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_PARSED, arktsGlobal.compilerContext.peer);
            this.logger.printDebug('es2panda proceedToState parsed');
            let ast = arkts.EtsScript.fromContext();
            if (this.buildConfig.aliasConfig && Object.keys(this.buildConfig.aliasConfig).length > 0) {
                // if aliasConfig is set, transform aliasName@kit.xxx to default@ohos.xxx through the plugin
                this.logger.printDebug('Transforming import statements with alias config');
                let transformAst = new KitImportTransformer(
                    arkts,
                    arktsGlobal.compilerContext.program,
                    this.buildConfig.buildSdkPath,
                    this.buildConfig.aliasConfig
                ).transform(ast);
                PluginDriver.getInstance().getPluginContext().setArkTSAst(transformAst);
            } else {
                PluginDriver.getInstance().getPluginContext().setArkTSAst(ast);
            }
            PluginDriver.getInstance().runPluginHook(PluginHook.PARSED);
            this.logger.printInfo('plugin parsed finished');

            arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_CHECKED, arktsGlobal.compilerContext.peer);
            this.logger.printInfo('es2panda proceedToState checked');

            if (job.type & CompileJobType.DECL) {
                // Generate 1.2 declaration files(a temporary solution while binary import not pushed)
                arkts.generateStaticDeclarationsFromContext(outputDeclFilePath);
                this.logger.printDebug("compile FINISH [DECL]")
            }
            if (job.type & CompileJobType.ABC) {
                ast = arkts.EtsScript.fromContext();
                PluginDriver.getInstance().getPluginContext().setArkTSAst(ast);
                PluginDriver.getInstance().runPluginHook(PluginHook.CHECKED);
                this.logger.printInfo('plugin checked finished');

                arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_BIN_GENERATED, arktsGlobal.compilerContext.peer);
                this.logger.printInfo('es2panda bin generated');
                this.logger.printDebug("compile FINISH [ABC]")
            }
        } catch (error) {
            if (error instanceof Error) {
                throw new DriverError(
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL,
                        'Compile abc files failed.',
                        error.message,
                        inputFilePath
                    )
                );
            }
        } finally {
            PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
            arktsGlobal.es2panda._DestroyContext(arktsGlobal.compilerContext.peer);
            arkts.destroyConfig(arktsGlobal.config);
        }
    }

    public async run(): Promise<void> {
        this.jobs = this.depAnalyzer.collectJobs(this.entryFiles, this.fileToModule, this.moduleInfos);
        if (this.es2pandaDepGraphDotDump) {
            fs.writeFileSync(path.resolve(this.cacheDir, 'graph.dot'), dotGraphDump(this.jobs), 'utf-8')
        }

        this.initCompileQueues();

        while (this.haveQueuedJobs()) {
            let job: CompileJobInfo = this.consumeJob()!
            if (job.fileList.length > 1) {
                // Compile cycle simultaneous
                this.logger.printDebug("Compiling cycle....")
                this.logger.printDebug(`file list: \n${job.fileList.join('\n')}`)
                this.compileSimultaneous(job)
            } else {
                this.compile(job)
            }
            this.dispatchNextJob(job)
        }
        if (this.completedJobQueue.length > 0) {
            this.mergeAbcFiles();
        }
    }

    public compileSimultaneous(job: CompileJobInfo): void {
        let compileSingleData = new CompileSingleData(path.join(path.resolve(), BS_PERF_FILE_NAME));
        compileSingleData.record(RECORDE_COMPILE_NODE.PROCEED_PARSE);

        this.logger.printDebug(`job ${JSON.stringify(job, null, 1)}`)

        const ets2pandaCmd: string[] = formEts2pandaCmd(job, this.isDebug, true)
        this.logger.printDebug('ets2pandaCmd: ' + ets2pandaCmd.join(' '));

        let { arkts, arktsGlobal } = this.koalaModule;
        try {
            arktsGlobal.config = arkts.Config.create(ets2pandaCmd).peer;
            arktsGlobal.compilerContext = arkts.Context.createContextGenerateAbcForExternalSourceFiles(job.fileList);
            PluginDriver.getInstance().getPluginContext().setArkTSProgram(arktsGlobal.compilerContext.program);

            arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_PARSED, arktsGlobal.compilerContext.peer);
            this.logger.printInfo('es2panda proceedToState parsed');
            compileSingleData.record(RECORDE_COMPILE_NODE.PLUGIN_PARSE, RECORDE_COMPILE_NODE.PROCEED_PARSE);

            let ast = arkts.EtsScript.fromContext();

            if (this.buildConfig.aliasConfig && Object.keys(this.buildConfig.aliasConfig).length > 0) {
                // if aliasConfig is set, transform aliasName@kit.xxx to default@ohos.xxx through the plugin
                this.logger.printInfo('Transforming import statements with alias config');
                let transformAst = new KitImportTransformer(
                    arkts,
                    arktsGlobal.compilerContext.program,
                    this.buildConfig.buildSdkPath,
                    this.buildConfig.aliasConfig
                ).transform(ast);
                PluginDriver.getInstance().getPluginContext().setArkTSAst(transformAst);
            } else {
                PluginDriver.getInstance().getPluginContext().setArkTSAst(ast);
            }

            PluginDriver.getInstance().getPluginContext().setArkTSAst(ast);
            PluginDriver.getInstance().runPluginHook(PluginHook.PARSED);
            this.logger.printInfo('plugin parsed finished');
            compileSingleData.record(RECORDE_COMPILE_NODE.PROCEED_CHECK, RECORDE_COMPILE_NODE.PLUGIN_PARSE);

            arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_CHECKED, arktsGlobal.compilerContext.peer);
            this.logger.printInfo('es2panda proceedToState checked');
            compileSingleData.record(RECORDE_COMPILE_NODE.PLUGIN_CHECK, RECORDE_COMPILE_NODE.PROCEED_CHECK);

            // NOTE: workaround to build arkoala arkui
            // NOTE: to be refactored
            if (job.type & CompileJobType.DECL) {
                for (const file of job.fileList) {
                    const module = this.fileToModule.get(file)!
                    const declEtsOutputPath: string = changeFileExtension(
                        path.resolve(this.cacheDir, module.packageName,
                            path.relative(module.moduleRootPath, file)
                        ),
                        DECL_ETS_SUFFIX
                    )

                    ensurePathExists(declEtsOutputPath);

                    arkts.generateStaticDeclarationsFromContext(declEtsOutputPath);
                }
            }

            if (job.type & CompileJobType.ABC) {
                ast = arkts.EtsScript.fromContext();
                PluginDriver.getInstance().getPluginContext().setArkTSAst(ast);
                PluginDriver.getInstance().runPluginHook(PluginHook.CHECKED);
                this.logger.printInfo('plugin checked finished');
                compileSingleData.record(RECORDE_COMPILE_NODE.BIN_GENERATE, RECORDE_COMPILE_NODE.PLUGIN_CHECK);

                arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_BIN_GENERATED, arktsGlobal.compilerContext.peer);
                this.logger.printInfo('es2panda bin generated');
                compileSingleData.record(RECORDE_COMPILE_NODE.CFG_DESTROY, RECORDE_COMPILE_NODE.BIN_GENERATE);
            }
        } catch (error) {
            if (error instanceof Error) {
                throw new DriverError(
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL,
                        'Compile abc files failed.',
                        error.message,
                        job.compileFileInfo.inputFilePath
                    )
                );
            }
        } finally {
            arktsGlobal.es2panda._DestroyContext(arktsGlobal.compilerContext.peer);
            PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
            arkts.destroyConfig(arktsGlobal.config);
            compileSingleData.record(RECORDE_COMPILE_NODE.END, RECORDE_COMPILE_NODE.CFG_DESTROY);
            compileSingleData.writeSumSingle(path.resolve());
        }
    }

    private mergeAbcFiles(): void {
        let linkerInputFile: string = path.join(this.cacheDir, LINKER_INPUT_FILE);
        let linkerInputContent: string = '';
        this.completedJobQueue.forEach((job: CompileJobInfo) => {
            if (job.compileFileInfo.outputFilePath.endsWith(ABC_SUFFIX)) {
                linkerInputContent += job.compileFileInfo.outputFilePath + os.EOL;
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
        let compileSingleData = new CompileSingleData(path.join(path.resolve(), BS_PERF_FILE_NAME));
        compileSingleData.record(RECORDE_RUN_NODE.GEN_MODULE);

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

        // We do not need any queues just compile a bunch of files
        // Ets2panda will build it simultaneous
        this.compileSimultaneous({
            id: outputFile,
            fileList: [...this.entryFiles],
            jobDependencies: [],
            jobDependants: [],
            compileFileInfo: {
                inputFilePath: entryFile,
                outputFilePath: outputFile,
                arktsConfigFile: arktsConfigFile
            },
            type: CompileJobType.ABC
        });
        compileSingleData.record(RECORDE_RUN_NODE.END, RECORDE_RUN_NODE.COMPILE_FILES);
        compileSingleData.writeSumSingle(path.resolve());
    }

    public async runParallel(): Promise<void> {
        // NOTE: TBD
        throw new DriverError(
            LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL,
                'Parallel mode is currently unavailable.'
            )
        )
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

    public async generateDeclaration(): Promise<void> {
        this.jobs = this.depAnalyzer.collectJobs(this.entryFiles, this.fileToModule, this.moduleInfos);
        this.initCompileQueues();

        while (this.haveQueuedJobs()) {
            let job: CompileJobInfo = this.consumeJob()!
            this.declgen(job)
            this.dispatchNextJob(job)
        }
    }

    private declgen(jobInfo: CompileJobInfo): void {
        const inputFilePath = jobInfo.compileFileInfo.inputFilePath;
        const source = fs.readFileSync(inputFilePath, 'utf8');
        const moduleInfo: ModuleInfo = this.fileToModule.get(inputFilePath)!;
        const filePathFromModuleRoot: string = path.relative(moduleInfo.moduleRootPath, inputFilePath);
        const declEtsOutputPath: string = changeDeclgenFileExtension(
            path.join(moduleInfo.declgenV1OutPath as string, moduleInfo.packageName, filePathFromModuleRoot),
            DECL_ETS_SUFFIX
        );
        const etsOutputPath: string = changeDeclgenFileExtension(
            path.join(moduleInfo.declgenBridgeCodePath as string, moduleInfo.packageName, filePathFromModuleRoot),
            TS_SUFFIX
        );
        ensurePathExists(declEtsOutputPath);
        ensurePathExists(etsOutputPath);

        let { arkts, arktsGlobal } = this.koalaModule;

        try {
            const staticRecordPath = path.join(
                moduleInfo.declgenV1OutPath as string,
                STATIC_RECORD_FILE
            )
            const declEtsOutputDir = path.dirname(declEtsOutputPath);
            const staticRecordRelativePath = changeFileExtension(
                path.relative(declEtsOutputDir, staticRecordPath).replace(/\\/g, '\/'),
                '',
                DECL_TS_SUFFIX
            );
            createFileIfNotExists(staticRecordPath, STATIC_RECORD_FILE_CONTENT);

            let ets2pandaCmd = formEts2pandaCmd(jobInfo)
            this.logger.printDebug(`ets2panda cmd: ${ets2pandaCmd.join(' ')}`)

            arktsGlobal.filePath = inputFilePath;
            arktsGlobal.config = arkts.Config.create(ets2pandaCmd).peer;
            arktsGlobal.compilerContext = arkts.Context.createFromStringWithHistory(source);
            PluginDriver.getInstance().getPluginContext().setArkTSProgram(arktsGlobal.compilerContext.program);

            arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_PARSED, arktsGlobal.compilerContext.peer, this.skipDeclCheck);

            let ast = arkts.EtsScript.fromContext();
            PluginDriver.getInstance().getPluginContext().setArkTSAst(ast);
            PluginDriver.getInstance().runPluginHook(PluginHook.PARSED);

            arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_CHECKED, arktsGlobal.compilerContext.peer, this.skipDeclCheck);

            ast = arkts.EtsScript.fromContext();
            PluginDriver.getInstance().getPluginContext().setArkTSAst(ast);
            PluginDriver.getInstance().runPluginHook(PluginHook.CHECKED);

            arkts.generateTsDeclarationsFromContext(
                declEtsOutputPath,
                etsOutputPath,
                false,
                false,
                staticRecordRelativePath,
                this.genDeclAnnotations
            ); // Generate 1.0 declaration files & 1.0 glue code
            this.logger.printInfo('declaration files generated');
        } catch (error) {
            if (error instanceof Error) {
                throw new DriverError(
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_DECLGEN_FAIL,
                        'Generate declaration files failed.',
                        error.message,
                        inputFilePath
                    )
                );
            }
        } finally {
            arktsGlobal.es2panda._DestroyContext(arktsGlobal.compilerContext.peer);
            arkts.destroyConfig(arktsGlobal.config);
        }
    }

    // NOTE: to be refactored
    /*
    protected collectAbcFileFromByteCodeHar(): void {
        // the abc of the dependent bytecode har needs to be included When compiling hsp/hap
        // but it's not required when compiling har
        if (this.buildConfig.moduleType === OHOS_MODULE_TYPE.HAR) {
            return;
        }
        for (const [packageName, moduleInfo] of this.moduleInfos) {
            if (!(moduleInfo.moduleType === OHOS_MODULE_TYPE.HAR && moduleInfo.byteCodeHar)) {
                continue;
            }
            if (moduleInfo.language === LANGUAGE_VERSION.ARKTS_1_1) {
                continue;
            }
            if (!moduleInfo.abcPath) {
                const logData: LogData = LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_ABC_FILE_MISSING_IN_BCHAR,
                    `abc file not found in bytecode har ${packageName}. `
                );
                this.logger.printError(logData);
                continue;
            }
            if (!fs.existsSync(moduleInfo.abcPath)) {
                const logData: LogData = LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_ABC_FILE_NOT_EXIST_IN_BCHAR,
                    `${moduleInfo.abcPath} does not exist. `
                );
                this.logger.printErrorAndExit(logData);
            }
            this.abcFiles.add(moduleInfo.abcPath);
        }
    }

    public async generateDeclarationParallell(): Promise<void> {
        this.processBuildConfig();

        const taskManager = new TaskManager<CompileTask>(handleCompileWorkerExit);

        try {
            taskManager.startWorkers(DriverProcess, path.resolve(__dirname, 'declgen_worker.js'));

            const taskPromises = Array.from(this.fileToModule.values()).map(task =>
                taskManager.submitTask({
                    fileInfo: task,
                    buildConfig: this.getSerializableConfig() as BuildConfig,
                    moduleInfos: Array.from(this.moduleInfos.entries())
                })
            );

            await Promise.all(taskPromises);

            this.logger.printInfo('All declaration generation tasks complete.');
        } catch (error) {
            this.logger.printError(LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_DECLGEN_FAIL,
                `Generate declaration files failed.\n${(error as Error)?.message || error}`
            ));
        } finally {
            await taskManager.shutdown();
        }
    }

    private assignTaskToIdleWorker(workerInfo: WorkerInfo, processingJobs: Set<string>, serializableConfig: Object, globalContextPtr: KPointer): void {

        let jobInfo = this.consumeJob();
        if (!jobInfo) {
            return;
        }

        processingJobs.add(jobInfo.id);
        workerInfo.worker.send('ASSIGN_TASK', jobInfo);
        workerInfo.isIdle = false;
    }

    private checkAllTasksDone(workerPool: WorkerInfo[]): boolean {
        if (this.jobQueue.length === 0) {
            for (let i = 0; i < workerPool.length; i++) {
                if (!workerPool[i].isIdle) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    private processAfterCompile(config: KPointer, globalContext: KPointer): void {

        let arktsGlobal = this.koalaModule.arktsGlobal;
        let arkts = this.koalaModule.arkts;

        arktsGlobal.es2panda._DestroyGlobalContext(globalContext);
        arkts.destroyConfig(config);
        arktsGlobal.es2panda._MemFinalize();

        this.mergeAbcFiles();
    }

    CC-OFFNXT(huge_depth)
    private async invokeWorkers(processingJobs: Set<string>, workers: DriverThread[]): Promise<void> {
      return new Promise<void>((resolve) => {
        const numWorkers = 1;

        let files: string[] = [];

        Object.entries(this.jobs).forEach(([key, job]) => {
          for (let i = 0; i < job.fileList.length; i++) {
            files.push(job.fileList[i]);
          }
        });

        let arkts = this.buildConfig.arkts;
        let fileInfo = this.fileToModule.values().next().value!;

        let ets2pandaCmd: string[] = [
          '_',
          '--extension',
          'ets',
          '--arktsconfig',
          fileInfo.arktsConfigFile,
          '--output',
          fileInfo.inputFilePath,
        ];

        if (this.isDebug) {
          ets2pandaCmd.push('--debug-info');
          ets2pandaCmd.push('--opt-level=0');
        }
        ets2pandaCmd.push(fileInfo.inputFilePath);

        arkts.MemInitialize();

        let config = arkts.Config.create(ets2pandaCmd).peer;

        let globalContextPtr = arkts.CreateGlobalContext(config, files, files.length, false);
        const serializableConfig = this.getSerializableConfig();

        const workerPool: WorkerInfo[] = [];
        for (let i = 0; i < numWorkers; i++) {
          const worker = new DriverThread(
            path.resolve(__dirname, 'compile_thread_worker.js'),
            { workerData: { workerId: i } }
          );

          workers.push(worker);
          workerPool.push({ worker, id: i, isIdle: true, isKilled: false  });
          this.assignTaskToIdleWorker(workerPool[i], processingJobs, serializableConfig, globalContextPtr);
          worker.on('message', (msg) => {
            if (msg.type === 'TASK_FINISH') {
              const workerInfo = workerPool.find(w => w.worker === worker);
              if (workerInfo) {
                workerInfo.isIdle = true;
              }
              const jobId = msg.jobId;
              finishedJob.push(jobId);
              processingJobs.delete(jobId);
              const completedJob = this.jobs[jobId];
              this.completeJob(completedJob);
              for (let j = 0; j < workerPool.length; j++) {
                if (workerPool[j].isIdle) {
                  this.assignTaskToIdleWorker(workerPool[j], processingJobs, serializableConfig, globalContextPtr);
                }
              }
            }
            if (this.checkAllTasksDone(workerPool)) {
              workers.forEach(worker => worker.send('EXIT'));
              this.processAfterCompile(config, globalContextPtr);
              resolve();
            }
          });
        }
      });
    }
    */
}
