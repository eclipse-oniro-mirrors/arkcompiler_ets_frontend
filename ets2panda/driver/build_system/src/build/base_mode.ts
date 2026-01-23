/*
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

import { Ets2panda } from '../util/ets2panda'
import {
    ARKTSCONFIG_JSON_FILE,
    LANGUAGE_VERSION,
    LINKER_INPUT_FILE,
    MERGED_ABC_FILE,
    CLUSTER_FILES_TRESHOLD,
    DECL_ETS_SUFFIX,
    MERGED_INTERMEDIATE_FILE,
    ENABLE_CLUSTERS,
    DEFAULT_WORKER_NUMS,
    DECL_FILE_MAP_NAME,
    TS_SUFFIX
} from '../pre_define';
import {
    ensurePathExists,
    isMac,
    checkDependencyModuleInfoCorrectness,
    changeDeclgenFileExtension
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
    OHOS_MODULE_TYPE,
    ModuleFile,
    DeclFileInfo
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
    private abcFiles: Set<string>;
    protected mergedAbcFile: string;
    protected logger: Logger;
    protected readonly statsRecorder: StatisticsRecorder;
    private readonly moduleType: OHOS_MODULE_TYPE;
    public declFileMap: Map<string, DeclFileInfo> = new Map<string, DeclFileInfo>();

    constructor(buildConfig: BuildConfig) {
        this.buildConfig = buildConfig;
        this.entryFiles = new Set<string>(buildConfig.compileFiles);
        this.fileToModule = new Map<string, ModuleInfo>();
        this.moduleInfos = new Map<string, ModuleInfo>();
        this.mergedAbcFile = path.resolve(this.outputDir, MERGED_ABC_FILE);
        this.logger = Logger.getInstance();
        this.abcFiles = new Set<string>();
        this.moduleType = buildConfig.moduleType;

        this.statsRecorder = new StatisticsRecorder(path.resolve(this.cacheDir, BS_PERF_FILE_NAME), this.recordType,
                                                    `Build system with mode: ${this.es2pandaMode}`);

        this.processBuildConfig();
        this.statsRecorder.record(formEvent(BuildSystemEvent.BACKWARD_COMPAT));
        this.backwardCompatibilityWorkaroundStub()
    }

    public loadDeclFileMap(): void {
        const declMapFile = path.join(this.cacheDir, DECL_FILE_MAP_NAME);
        if (!fs.existsSync(declMapFile)) {
            return;
        }
        try {
            const content = fs.readFileSync(declMapFile, 'utf-8');
            const data: Record<string, DeclFileInfo> = JSON.parse(content);

            for (const [key, value] of Object.entries(data)) {
                this.declFileMap.set(key, value);
            }
        } catch (error) {
            const logData = LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_ERRORS_OCCURRED,
                `Failed to load decl file map from ${declMapFile}.`,
                error instanceof Error ? error.message : String(error)
            );
            this.logger.printError(logData);
        }
    }

    private needsRegeneration(job: DeclgenV1JobInfo): boolean {
        const sourceFilePath = job.fileList[0];
        const sourceStat = fs.statSync(sourceFilePath);
        const currentModified = sourceStat.mtimeMs;
        const fileInfo = this.declFileMap.get(sourceFilePath);
        if (!fileInfo || fileInfo.sourceFileLastModified === null) {
            return true;
        }
        return currentModified > fileInfo.sourceFileLastModified;
    }

    public async saveDeclFileMap(): Promise<void> {
        const declMapFile = path.join(this.cacheDir, DECL_FILE_MAP_NAME);
        const data: Record<string, DeclFileInfo> = {};
        this.declFileMap.forEach((value, key) => {
            data[key] = value;
        });
        try {
            await fs.promises.mkdir(path.dirname(declMapFile), { recursive: true });
            await fs.promises.writeFile(declMapFile, JSON.stringify(data, null, 2));
        } catch (error) {
            this.logger.printError(LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_ERRORS_OCCURRED,
                `Failed to save decl file map to disk.`,
                error instanceof Error ? error.message : String(error)
            ));
            throw error;
        }
    }

    public getOutputFilePaths(job: DeclgenV1JobInfo): {declEtsOutputPath: string, glueCodeOutputPath: string} {
        const inputFilePath = job.fileInfo.input;
        const filePathFromModuleRoot: string = path.relative(job.fileInfo.moduleRoot, inputFilePath);
        const declEtsOutputPath: string = changeDeclgenFileExtension(
            path.resolve(job.declgenConfig.output, job.fileInfo.moduleName, filePathFromModuleRoot), DECL_ETS_SUFFIX);
        const glueCodeOutputPath: string = changeDeclgenFileExtension(
            path.resolve(job.declgenConfig.bridgeCode, job.fileInfo.moduleName, filePathFromModuleRoot), TS_SUFFIX);
        ensurePathExists(declEtsOutputPath);
        ensurePathExists(glueCodeOutputPath);
        return {declEtsOutputPath, glueCodeOutputPath};
    }

    public async needsBackup(job: DeclgenV1JobInfo): Promise<{needsDeclBackup: boolean; needsGlueCodeBackup: boolean}> {
        const {declEtsOutputPath, glueCodeOutputPath} = this.getOutputFilePaths(job);
        let needsDeclBackup = false;
        let needsGlueCodeBackup = false;
        const declInfo = this.declFileMap.get(job.fileList[0]);
        const isFileExists = async(path: string): Promise<boolean> => {
            try {
                const stat = await fs.promises.stat(path);
                return stat.isFile();
            } catch {
                return false;
            }
        };
        const [declFileExists, glueCodeExists] =
            await Promise.all([isFileExists(declEtsOutputPath), isFileExists(glueCodeOutputPath)]);
        if (declFileExists) {
            if (declInfo?.declLastModified != null) {
                const declStat = await fs.promises.stat(declEtsOutputPath);
                const declModified = declStat.mtimeMs;
                if (declModified > declInfo.declLastModified) {
                    needsDeclBackup = true;
                }
            }
        }
        if (glueCodeExists) {
            if (declInfo?.glueCodeLastModified != null) {
                const glueStat = await fs.promises.stat(glueCodeOutputPath);
                const glueCodeModified = glueStat.mtimeMs;
                if (glueCodeModified > declInfo.glueCodeLastModified) {
                    needsGlueCodeBackup = true;
                }
            }
        }
        return {needsDeclBackup, needsGlueCodeBackup};
    }

    public async backupFiles(job: DeclgenV1JobInfo, needsDecl: boolean, needsGlue: boolean): Promise<void> {
        const { declEtsOutputPath, glueCodeOutputPath } = this.getOutputFilePaths(job);
        const doCopy = async (filePath: string, type: string): Promise<void> => {
            if (!fs.existsSync(filePath)) {
                return;
            }
            const backupPath = `${filePath}.backup`;
            try {
                await fs.promises.copyFile(filePath, backupPath);
                this.logger.printDebug(`Backup completed for ${type}: ${backupPath}`);
            } catch (error) {
                const logData = LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_ERRORS_OCCURRED,
                    `Critical: Failed to backup ${type} at ${filePath}.`,
                    error instanceof Error ? error.message : String(error)
                );
                this.logger.printError(logData);
                throw error;
            }
        };

        const backups = [];
        if (needsDecl) {
            backups.push(doCopy(declEtsOutputPath, 'declaration file'));
        }
        if (needsGlue) {
            backups.push(doCopy(glueCodeOutputPath, 'glue code file'));
        }
        await Promise.all(backups);
    }

    public async updateDeclFileMapAsync(job: DeclgenV1JobInfo): Promise<void> {
        const sourceFilePath = job.fileList[0];
        const { declEtsOutputPath, glueCodeOutputPath } = this.getOutputFilePaths(job);
        const [sourceFileStat, declStat, glueCodeStat] = await Promise.all([
            fs.promises.stat(sourceFilePath),
            fs.promises.stat(declEtsOutputPath).catch(() => null),
            fs.promises.stat(glueCodeOutputPath).catch(() => null)
        ]);

        if (declStat || glueCodeStat) {
            this.declFileMap.set(sourceFilePath, {
                delFilePath: declEtsOutputPath,
                declLastModified: declStat?.mtimeMs ?? null,
                glueCodeFilePath: glueCodeOutputPath,
                glueCodeLastModified: glueCodeStat?.mtimeMs ?? null,
                sourceFilePath: sourceFilePath,
                sourceFileLastModified: sourceFileStat.mtimeMs ?? null,
            });
            this.logger.printDebug(`Updated decl file map for: ${sourceFilePath}`);
        } else {
            const detail = `Expected outputs missing: ${declEtsOutputPath} or ${glueCodeOutputPath}`;
            const logData = LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_ERRORS_OCCURRED,
                `Build integrity error for ${sourceFilePath}`,
                detail
            );
            this.logger.printError(logData);
            throw new Error(detail);
        }
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
        job.type = (this.moduleType === OHOS_MODULE_TYPE.HAR || this.moduleType === OHOS_MODULE_TYPE.SHARED) ? CompileJobType.DECL_ABC : job.type;
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
            .map((nodeId) => {
                return buildGraph.getNodeById(nodeId);
            })
            .map((node) => {
                return { id: node.id, job: node.data };
            })

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
        job.type = (this.moduleType === OHOS_MODULE_TYPE.HAR || this.moduleType === OHOS_MODULE_TYPE.SHARED) ? CompileJobType.DECL_ABC : job.type;
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

    private collectAbcFileFromByteCodeHar(): void {
        // the abc of the dependent bytecode har needs to be included when compiling hsp/hap
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
                const logData = LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_ABC_FILE_MISSING_IN_BCHAR, `abc file not found in bytecode har ${packageName}. `);
                this.logger.printError(logData);
                continue;
            }
            if (!fs.existsSync(moduleInfo.abcPath)) {
                const logData = LogDataFactory.newInstance(ErrorCode.BUILDSYSTEM_ABC_FILE_NOT_EXIST_IN_BCHAR, `${moduleInfo.abcPath} does not exist. `);
                this.logger.printErrorAndExit(logData);
            }
            this.abcFiles.add(moduleInfo.abcPath);
        }
    }

    private mergeAbcFiles(outPuts: string[] = []): void {
        this.collectAbcFileFromByteCodeHar();
        let linkerInputFile: string = path.join(this.cacheDir, LINKER_INPUT_FILE);
        let allFiles: string[] = outPuts.concat(Array.from(this.abcFiles));
        let linkerInputContent: string = allFiles.join(os.EOL);
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

        const dependenciesSets = new Map<string, Set<string>>;

        // Fill dependenciesSets and generate ArktsConfigs
        this.moduleInfos.forEach((moduleInfo: ModuleInfo) => {
            dependenciesSets.set(moduleInfo.packageName, new Set())
            moduleInfo.dependencies?.forEach((dependency: string) => {
                dependenciesSets.get(moduleInfo.packageName)!.add(dependency)
            });
            ArkTSConfigGenerator.getInstance().generateArkTSConfigFile(moduleInfo, this.enableDeclgenEts2Ts);
        });

        // Merge ArktsConfigs
        // Start the recursive merge from the main module
        let arktsConfig = ArkTSConfigGenerator.getInstance().getArktsConfigByPackageName(this.mainPackageName)!;
        arktsConfig.mergeArktsConfigByDependencies(dependenciesSets.get(this.mainPackageName)!, dependenciesSets!);

        dependenciesSets.forEach((_: Set<string>, module: string) => {
            let moduleInfo = this.moduleInfos.get(module)!;
            let arktsConfig = ArkTSConfigGenerator.getInstance().getArktsConfigByPackageName(module)!;
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

    // ModuleFiles contain static file information of hsp and the packages that hsp depends on
    // This field only serves hsp
    protected collectModuleFiles(): void {
        if (!this.buildConfig.moduleFiles || this.buildConfig.moduleFiles.length === 0) {
            return;
        }

        const moduleFiles = this.buildConfig.moduleFiles;
        for (const moduleFile of moduleFiles) {
            let packageName = moduleFile.packageName;
            if (!this.moduleInfos.has(packageName)) {
                throw new DriverError(
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_PACKAGENAME_NOT_INCLUDED_IN_MODULEINFOS,
                        `Package '${packageName}' is not included in moduleInfos.`
                    )
                );
            }
            this.moduleInfos.get(packageName)!.staticFiles = moduleFile.staticFiles;
        }
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
                staticFiles: []
            };
            this.moduleInfos.set(dependency.packageName, moduleInfo);
            this.moduleInfos.get(this.mainPackageName)!.dependencies.push(dependency.packageName)
        });

        this.collectModuleDependencies();
    }

    protected getMainModuleInfo(): ModuleInfo {
        const mainModuleInfo = this.dependencyModuleList.find((module) =>
            module.packageName === this.mainPackageName
        );
        return {
            isMainModule: true,
            packageName: this.mainPackageName,
            moduleRootPath: mainModuleInfo?.modulePath ?? this.mainModuleRootPath,
            moduleType: mainModuleInfo?.moduleType ?? this.moduleType,
            sourceRoots: this.mainSourceRoots,
            entryFile: this.entryFile ?? '',
            arktsConfigFile: path.resolve(this.cacheDir, this.mainPackageName, ARKTSCONFIG_JSON_FILE),
            dynamicDependencyModules: new Map<string, ModuleInfo>(),
            staticDependencyModules: new Map<string, ModuleInfo>(),
            declgenV1OutPath: mainModuleInfo?.declgenV1OutPath ?? this.declgenV1OutPath,
            declgenV2OutPath: mainModuleInfo?.declgenV2OutPath ?? this.declgenV2OutPath,
            declgenBridgeCodePath: mainModuleInfo?.declgenBridgeCodePath ?? this.declgenBridgeCodePath,
            byteCodeHar: this.byteCodeHar,
            language: mainModuleInfo?.language ?? LANGUAGE_VERSION.ARKTS_1_2,
            declFilesPath: mainModuleInfo?.declFilesPath,
            dependencies: mainModuleInfo?.dependencies ?? [],
            staticFiles: []
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
        if (!this.buildConfig.enableDeclgenEts2Ts) {
            this.entryFiles = new Set([...this.entryFiles].filter(file => !file.endsWith(DECL_ETS_SUFFIX)));
        }
        this.logger.printDebug(`collected fileToModule ${JSON.stringify([...this.fileToModule.entries()], null, 1)}`)
    }

    protected processBuildConfig(): void {
        this.statsRecorder.record(formEvent(BuildSystemEvent.COLLECT_MODULES));
        this.collectModuleInfos();
        this.collectModuleFiles();
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
        }).map(([file, _]: [string, ModuleInfo]) => {
            return file
        })
        mainModule.entryFile = mainModuleFileList[0]

        this.logger.printDebug(`mainModule entryFile: ${mainModule.entryFile}`)
    }

    public async runSimultaneous(): Promise<void> {
        this.statsRecorder.record(formEvent(BuildSystemEvent.RUN_SIMULTANEOUS));

        const mainModule: ModuleInfo = this.moduleInfos.get(this.mainPackageName)!;
        let entryFile: string = mainModule.entryFile || [...this.entryFiles][0];
        let arktsConfigFile: string = mainModule.arktsConfigFile;
        let intermediateFilePath: string = path.resolve(this.cacheDir, MERGED_INTERMEDIATE_FILE);
        this.logger.printDebug(`entryFile: ${entryFile}`)
        this.logger.printDebug(`module: ${JSON.stringify(mainModule, null, 1)}`)
        this.logger.printDebug(`arktsConfigFile: ${arktsConfigFile}`)

        // Just to init
        Ets2panda.getInstance(this.buildConfig)
        // We do not need any queues just compile a bunch of files
        // Ets2panda will build it simultaneous
        let res = this.compileSimultaneous('SimultaneousBuildId', {
            fileList: [...this.entryFiles],
            fileInfo: {
                input: entryFile,
                output: intermediateFilePath,
                arktsConfig: arktsConfigFile,
                moduleName: mainModule.packageName,
                moduleRoot: mainModule.moduleRootPath
            },
            declgenConfig: {
                output: mainModule.declgenV2OutPath!
            },
            type: CompileJobType.ABC
        });
        Ets2panda.destroyInstance();
        if (!res) {
            throw new Error('Simultaneous build failed.');
        }
        this.statsRecorder.record(formEvent(BuildSystemEvent.RUN_LINKER));
        this.mergeAbcFiles([intermediateFilePath]);
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
            const newType = (this.moduleType === OHOS_MODULE_TYPE.HAR || this.moduleType === OHOS_MODULE_TYPE.SHARED)
                ? CompileJobType.DECL_ABC : node.data.type;
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
            .map((nodeId: string) => {
                return buildGraph.getNodeById(nodeId);
            })
            .map((node) => {
                return node.data;
            });

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
        this.loadDeclFileMap();
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
        const declgenJobs: DeclgenV1JobInfo[] = [];
        for (const node of buildGraph.nodes) {
            const module = this.fileToModule.get(node.data.fileList[0]!)!
            const declgenV1OutPath: string = module.declgenV1OutPath!
            const declgenBridgeCodePath: string = module.declgenBridgeCodePath!
            const declgenJob: DeclgenV1JobInfo = {
                        ...node.data,
                        declgenConfig: {output: declgenV1OutPath, bridgeCode: declgenBridgeCodePath}
                    };
            if(this.needsRegeneration(declgenJob)){
                newNodes.push({
                id: node.id,
                data: {...declgenJob, buildConfig: this.buildConfig},
                predecessors: node.predecessors,
                descendants: node.descendants,
                });
                declgenJobs.push(declgenJob);
            }
        }
        await Promise.all(declgenJobs.map(async (declgenJob) => {
            const {needsDeclBackup, needsGlueCodeBackup} = await this.needsBackup(declgenJob);
                if (needsDeclBackup || needsGlueCodeBackup) {
                    await this.backupFiles(declgenJob, needsDeclBackup, needsGlueCodeBackup);
                }
        }));
        const newGraph: Graph<ProcessDeclgenV1Task> = Graph.createGraphFromNodes(newNodes);

        taskManager.buildGraph = newGraph;
        taskManager.initTaskQueue();
        // Ignore the result
        await taskManager.finish();
        await Promise.all(declgenJobs.map(async (declgenJob) => {
            await this.updateDeclFileMapAsync(declgenJob);
        }));
        await this.saveDeclFileMap();
    }
}
