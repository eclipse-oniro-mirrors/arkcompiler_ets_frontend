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
    DECL_ETS_SUFFIX,
    MERGED_INTERMEDIATE_FILE,
    DEFAULT_WORKER_NUMS,
    DECL_FILE_MAP_NAME,
    ETSCACHE_SUFFIX
} from '../pre_define';
import {
    ensurePathExists,
    isMac,
    checkDependencyModuleInfoCorrectness,
    buildDeclgenOutputPath,
    traverseDirAndFindFilesWithRegExp
} from '../util/utils';
import {
    Logger,
    LogDataFactory,
} from '../logger';
import { DependencyAnalyzer } from '../dependency_analyzer';
import { ErrorCode, DriverError } from '../util/error';
import {
    BuildConfig,
    DependencyModuleConfig,
    ModuleInfo,
    ProcessCompileTask,
    ProcessDeclgenV1Task,
    CompileJobInfo,
    CompileJobType,
    DeclgenV1JobInfo,
    ES2PANDA_MODE,
    OHOS_MODULE_TYPE,
    DeclFileInfo,
    JobContentType,
    FileInfo
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
    protected readonly arktsConfigGenerator: ArkTSConfigGenerator;
    public declFileMap: Map<string, DeclFileInfo> = new Map<string, DeclFileInfo>();

    constructor(buildConfig: BuildConfig) {
        this.buildConfig = buildConfig;
        this.entryFiles = new Set<string>(buildConfig.compileFiles);
        this.fileToModule = new Map<string, ModuleInfo>();
        this.moduleInfos = new Map<string, ModuleInfo>();
        this.mergedAbcFile = path.resolve(this.outputDir, MERGED_ABC_FILE);
        this.logger = Logger.getInstance();
        this.abcFiles = new Set<string>();
        this.arktsConfigGenerator = new ArkTSConfigGenerator(buildConfig);

        this.statsRecorder = new StatisticsRecorder(path.resolve(this.cacheDir, BS_PERF_FILE_NAME), this.recordType,
                                                    `Build system with mode: ` +
                                                    `${this.es2pandaMode ?? ES2PANDA_MODE.RUN_PARALLEL}`);

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

    private needsRegeneration(sourceFilePath: string): boolean {
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

    public async needsBackup(
        file: string
    ): Promise<{ needsDeclBackup: boolean; needsGlueCodeBackup: boolean }> {
        const moduleInfo = this.fileToModule.get(file);
        if (!moduleInfo) {
            return { needsDeclBackup: false, needsGlueCodeBackup: false };
        }
        const { declEtsOutputPath, glueCodeOutputPath } = buildDeclgenOutputPath(file, moduleInfo, this.cacheDir);
        let needsDeclBackup = false;
        let needsGlueCodeBackup = false;
        const declInfo = this.declFileMap.get(file);

        const isFileExists = async (path: string): Promise<boolean> => {
            try {
                const stat = await fs.promises.stat(path);
                return stat.isFile();
            } catch {
                return false;
            }
        };
        const [declFileExists, glueCodeExists] =
            await Promise.all([isFileExists(declEtsOutputPath), isFileExists(glueCodeOutputPath)]);

        if (declFileExists && declInfo?.declLastModified != null) {
            const declStat = await fs.promises.stat(declEtsOutputPath);
            needsDeclBackup = declStat.mtimeMs > declInfo.declLastModified;
        }

        if (glueCodeExists && declInfo?.glueCodeLastModified != null) {
            const glueStat = await fs.promises.stat(glueCodeOutputPath);
            needsGlueCodeBackup = glueStat.mtimeMs > declInfo.glueCodeLastModified;
        }

        return { needsDeclBackup, needsGlueCodeBackup };
    }

    public async backupFiles(
        file: string,
        needsDecl: boolean,
        needsGlue: boolean
    ): Promise<void> {
        const moduleInfo = this.fileToModule.get(file);
        if (!moduleInfo) {
            return;
        }
        const { declEtsOutputPath, glueCodeOutputPath } = buildDeclgenOutputPath(file, moduleInfo, this.cacheDir);

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

    public async updateDeclFileMapAsync(
        file: string
    ): Promise<void> {
        const moduleInfo = this.fileToModule.get(file);
        if (!moduleInfo) {
            return;
        }
        const { declEtsOutputPath, glueCodeOutputPath } =
            buildDeclgenOutputPath(file, moduleInfo, this.cacheDir);

        const [sourceFileStat, declStat, glueCodeStat] = await Promise.all([
            fs.promises.stat(file), fs.promises.stat(declEtsOutputPath).catch(() => null),
            fs.promises.stat(glueCodeOutputPath).catch(() => null)
        ]);

        if (declStat || glueCodeStat) {
            this.declFileMap.set(file, {
                delFilePath: declEtsOutputPath,
                declLastModified: declStat?.mtimeMs ?? null,
                glueCodeFilePath: glueCodeOutputPath,
                glueCodeLastModified: glueCodeStat?.mtimeMs ?? null,
                sourceFilePath: file,
                sourceFileLastModified: sourceFileStat.mtimeMs ?? null,
            });
            this.logger.printDebug(`Updated decl file map for: ${file}`);
        } else {
            const detail = `Expected outputs missing: ${declEtsOutputPath} or ${glueCodeOutputPath}`;
            const logData = LogDataFactory.newInstance(
                ErrorCode.BUILDSYSTEM_ERRORS_OCCURRED,
                `Build integrity error for ${file}`,
                detail
            );
            this.logger.printError(logData);
            throw new Error(detail);
        }
    }

    private async backupFileIfNeeded(file: string): Promise<void> {
        const { needsDeclBackup, needsGlueCodeBackup } = await this.needsBackup(file);
        if (needsDeclBackup || needsGlueCodeBackup) {
            await this.backupFiles(file, needsDeclBackup, needsGlueCodeBackup);
        }
    }

    private async backupDeclgenFiles(declgenJobs: DeclgenV1JobInfo[]): Promise<void> {
        const tasks: Array<Promise<void>> = [];
        for (const declgenJob of declgenJobs) {
            const contentFiles: FileInfo[] = declgenJob.contentType === JobContentType.FILE
                ? [declgenJob.content as FileInfo]
                : declgenJob.content as FileInfo[];
            for (const fileInfo of contentFiles) {
                tasks.push(this.backupFileIfNeeded(fileInfo.input));
            }
        }
        await Promise.all(tasks);
    }

    private async updateDeclFileMapForJobs(declgenJobs: DeclgenV1JobInfo[]): Promise<void> {
        const tasks: Array<Promise<void>> = [];
        for (const declgenJob of declgenJobs) {
            const contentFiles: FileInfo[] = declgenJob.contentType === JobContentType.FILE
                ? [declgenJob.content as FileInfo]
                : declgenJob.content as FileInfo[];
            for (const fileInfo of contentFiles) {
                tasks.push(this.updateDeclFileMapAsync(fileInfo.input));
            }
        }
        await Promise.all(tasks);
    }

    private buildJobFileToModuleMap(contentFiles: FileInfo[]): Record<string, ModuleInfo> {
        const jobFileToModuleMap: Record<string, ModuleInfo> = {};
        for (const fileInfo of contentFiles) {
            const fileModule = this.fileToModule.get(fileInfo.input);
            if (fileModule) {
                jobFileToModuleMap[fileInfo.input] = fileModule;
            }
        }
        return jobFileToModuleMap;
    }

    private nodeNeedsRegeneration(node: GraphNode<CompileJobInfo>): boolean {
        const checkImpl = (file: string) => {
            if (this.needsRegeneration(file)) {
                return true;
            }
            return false;
        }

        if (node.data.contentType === JobContentType.FILE) {
            return checkImpl((node.data.content as FileInfo).input);
        }

        for (const fi of (node.data.content as FileInfo[])) {
            if (checkImpl(fi.input)) {
                return true;
            }
        }
        return false;
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

    public get recordType(): 'OFF' | 'ON' | undefined {
        return this.buildConfig.recordType
    }

    public get enableDebugOutput(): boolean | undefined {
        return this.buildConfig.enableDebugOutput;
    }

    private compile(id: string, job: CompileJobInfo, incremental: boolean = true): boolean {
        const ets2panda = Ets2panda.getInstance();
        let errOccurred = false;
        ets2panda.initalize();
        try {
            ets2panda.compile(id, job, incremental)
        } catch (error) {
            if (error instanceof DriverError) {
                // Report the error
                this.logger.printError(error.logData);
                errOccurred = true;
            } else {
                // Propagate the error further
                throw error;
            }

        } finally {
            ets2panda.finalize()
        }

        return !errOccurred;
    }

    public async run(): Promise<void> {
        this.statsRecorder.record(formEvent(BuildSystemEvent.DEPENDENCY_ANALYZER));
        const depAnalyzer = new DependencyAnalyzer(this.buildConfig, this.arktsConfigGenerator);
        const allOutputs: string[] = [];
        const buildGraph = depAnalyzer.getGraph(this.entryFiles, this.fileToModule, this.moduleInfos, allOutputs);
        if (!buildGraph.hasNodes()) {
            this.logger.printWarn('Nothing to compile. Exiting...')
            return;
        }

        if (this.enableDebugOutput) {
            for (const node of buildGraph.nodes) {
                this.logger.printDebug(`DepGraph node: ${node.id}: ${JSON.stringify(node.data, null, 1)}`);
            }
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
            if (job.contentType === JobContentType.CLUSTER) {
                // Compile cluster simultaneously
                this.logger.printDebug('Compiling cluster....')
                this.logger.printDebug(`${(job.content as FileInfo[]).map((fi: FileInfo) => fi.input)}`)
            } else {
                this.logger.printDebug('Compiling file....')
                this.logger.printDebug(`${(job.content as FileInfo).input}`)
            }
            const res: boolean = this.compile(id, job)
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

        this.statsRecorder.record(formEvent(BuildSystemEvent.RUN_LINKER));
        this.mergeAbcFiles(allOutputs);
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
        if (allFiles.length === 0) {
            // if a 1.1 har rely on a 1.2 bytecode har, there will be no output files to link
            return;
        }
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

    private resolvePackageName(packageName: string, originalPackageNameMap?: Map<string, string>): string {
        return originalPackageNameMap?.get(packageName) ?? packageName;
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
        const dependenciesSets = new Map<string, Set<string>>;

        // Fill dependenciesSets and generate ArktsConfigs
        this.moduleInfos.forEach((moduleInfo: ModuleInfo) => {
            dependenciesSets.set(moduleInfo.packageName, new Set())
            moduleInfo.dependencies?.forEach((dependency: string) => {
                dependenciesSets.get(moduleInfo.packageName)!.add(dependency)
            });
            this.arktsConfigGenerator.generateArkTSConfigFile(moduleInfo, this.enableDeclgenEts2Ts);
        });

        // Merge ArktsConfigs
        // Start the recursive merge from the main module
        let arktsConfig = this.arktsConfigGenerator.getArktsConfigByPackageName(this.mainPackageName)!;
        arktsConfig.mergeArktsConfigByDependencies(dependenciesSets.get(this.mainPackageName)!, dependenciesSets!, this.arktsConfigGenerator);

        dependenciesSets.forEach((_: Set<string>, module: string) => {
            let moduleInfo = this.moduleInfos.get(module)!;
            let arktsConfig = this.arktsConfigGenerator.getArktsConfigByPackageName(module)!;
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
                moduleName: dependency.moduleName,
                packageName: dependency.packageName,
                bundleName: dependency.bundleName,
                bundleType: dependency.bundleType,
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
                dependencies: [],
                byteCodeHar: dependency.byteCodeHar,
                abcPath: dependency.abcPath,
                staticFiles: [],
                packageVersion: dependency.packageVersion,
                originalPackageNameMap: dependency.originalPackageNameMap
            };
            moduleInfo.dependencies = dependency.dependencies?.map(dep => this.resolvePackageName(dep, moduleInfo.originalPackageNameMap)) ?? [];
            this.moduleInfos.set(dependency.packageName, moduleInfo);
            this.moduleInfos.get(this.mainPackageName)!.dependencies.push(dependency.packageName)
        });

        this.collectModuleDependencies();
    }

    protected getMainModuleInfo(): ModuleInfo {
        const mainModuleInfo = this.dependencyModuleList.find((module) =>
            module.packageName === this.mainPackageName
        );
        let moduleInfo: ModuleInfo = {
            isMainModule: true,
            moduleName: mainModuleInfo?.moduleName,
            packageName: this.mainPackageName,
            bundleName: mainModuleInfo?.bundleName,
            bundleType: mainModuleInfo?.bundleType,
            moduleRootPath: mainModuleInfo?.modulePath ?? this.mainModuleRootPath,
            moduleType: mainModuleInfo?.moduleType ?? this.mainModuleType,
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
            dependencies: [],
            staticFiles: [],
            packageVersion: mainModuleInfo?.packageVersion,
            originalPackageNameMap: mainModuleInfo?.originalPackageNameMap
        };
        moduleInfo.dependencies = mainModuleInfo?.dependencies?.map(dep => this.resolvePackageName(dep, moduleInfo.originalPackageNameMap)) ?? [];
        return moduleInfo;
    }

    protected processEntryFiles(): void {
        this.entryFiles.forEach((file: string) => {
            if (this.fileToModule.has(path.resolve(file))) {
                return;
            }
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
        if (!this.buildConfig.enableDeclgenEts2Ts && !this.buildConfig.frameworkMode) {
            this.entryFiles = new Set([...this.entryFiles].filter(file => !file.endsWith(DECL_ETS_SUFFIX) && !file.endsWith(ETSCACHE_SUFFIX)));
        }
        this.logger.printDebug(`collected fileToModule ${JSON.stringify([...this.fileToModule.entries()], null, 1)}`)
    }

    protected processBuildConfig(): void {
        this.statsRecorder.record(formEvent(BuildSystemEvent.COLLECT_MODULES));
        this.collectModuleInfos();
        this.collectModuleFiles();
        // called here, since processing of entryFiles goes further in processEntryFiles
        this.extractDeclarationsFromAbcFile();
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

        if (this.entryFiles.size === 0) {
            this.statsRecorder.record(formEvent(BuildSystemEvent.RUN_LINKER));
            // if there is no entry files, just need to merge the abc files of har packages
            // that may be relied on, e.g., a 1.1 hap relying on a 1.2 bytecode har
            this.mergeAbcFiles([]);
            return;
        }
        const mainModule: ModuleInfo = this.moduleInfos.get(this.mainPackageName)!;
        let arktsConfigFile: string = mainModule.arktsConfigFile;
        const content: FileInfo[] = []
        this.entryFiles.forEach((file: string) => { content.push({ input: file, output: '' }); });

        // Just to init
        Ets2panda.getInstance(this.buildConfig)
        const res = this.compile('SimultaneousBuildId', {
            contentType: JobContentType.CLUSTER,
            content: content,
            arktsConfig: arktsConfigFile,
            moduleName: mainModule.packageName,
            moduleRoot: mainModule.moduleRootPath,
            declgenConfig: {
                output: mainModule.declgenV2OutPath!
            },
            jobType: CompileJobType.ABC
        }, false);
        Ets2panda.destroyInstance();

        if (!res) {
            throw new Error('Simultaneous build failed.');
        }

        this.statsRecorder.record(formEvent(BuildSystemEvent.RUN_LINKER));
        this.mergeAbcFiles([path.resolve(this.cacheDir, MERGED_INTERMEDIATE_FILE)]);
    }

    public async runParallel(): Promise<void> {
        if (this.entryFiles.size === 0) {
            // in case of ENABLE_CLUSTERS is set to false and there is no entry files
            await this.runSimultaneous();
            return;
        }
        this.statsRecorder.record(formEvent(BuildSystemEvent.DEPENDENCY_ANALYZER));
        const depAnalyzer = new DependencyAnalyzer(this.buildConfig, this.arktsConfigGenerator);
        const allOutputs: string[] = [];
        const buildGraph = depAnalyzer.getGraph(this.entryFiles, this.fileToModule, this.moduleInfos, allOutputs);
        if (!buildGraph.hasNodes()) {
            this.logger.printWarn('Nothing to compile. Exiting...')
            this.mergeAbcFiles(allOutputs);
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
            newNodes.push({
                id: node.id,
                data: {
                    ...node.data,
                    buildConfig: this.buildConfig
                },
                predecessors: node.predecessors,
                descendants: node.descendants,
            })
        }
        const newGraph: Graph<ProcessCompileTask> = Graph.createGraphFromNodes(newNodes);

        if (this.enableDebugOutput) {
            for (const node of buildGraph.nodes) {
                this.logger.printDebug(`DepGraph node: ${node.id}: ${JSON.stringify(node.data.content, null, 1)}`);
            }
        }

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
        let declgenJob: DeclgenV1JobInfo = {
            ...job,
            fileToModuleMap: Object.fromEntries(this.fileToModule.entries())
        };

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
        const depAnalyzer = new DependencyAnalyzer(this.buildConfig, this.arktsConfigGenerator);
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

    private extractDeclarationsFromAbcFile(): void {
        if (this.buildConfig.dependentModuleList === undefined) {
            return;
        }
        const ets2panda: Ets2panda = Ets2panda.getInstance(this.buildConfig)
        const currentPackage = this.mainPackageName;
        const currentPackageDependenciesNames = this.buildConfig.dependentModuleList.find(pack => pack.packageName === currentPackage)?.dependencies;
        const currentPackageDependencies = this.buildConfig.dependentModuleList.filter(pack => currentPackageDependenciesNames?.includes(pack.packageName));
        let extractedDeclarations: string[] = [];
        // transitive dependencies currently are not processed
        for (const dep of currentPackageDependencies) {
            // better to move this check to projectionConfig validation
            if (dep.language != LANGUAGE_VERSION.ARKTS_1_2 && dep.language != LANGUAGE_VERSION.ARKTS_HYBRID) {
                continue;
            }
            if (dep.abcPath) {
                // this.cacheDir points to the cache for current package
                // etscache should be stored in the cache for the whole application,
                // since several packages may depend on the same package
                ets2panda.extractDeclarationsFromAbcFile(dep.abcPath, this.cacheDir);
                const packageCache = path.resolve(this.cacheDir, dep.packageName);
                const moduleInfo = this.moduleInfos.get(dep.packageName);
                const currentDecls = traverseDirAndFindFilesWithRegExp(packageCache, /\.etscache$/);
                for (const decl of currentDecls) {
                    if (moduleInfo) {
                        this.fileToModule.set(decl, moduleInfo);
                    }
                }
                extractedDeclarations = extractedDeclarations.concat(currentDecls);
            }
        }
        Ets2panda.destroyInstance();

        if (extractedDeclarations.length > 0) {
            this.entryFiles = new Set<string>([...this.entryFiles, ...extractedDeclarations]);
        }
    }

    public async generateDeclarationV1Parallel(): Promise<void> {
        this.statsRecorder.record(formEvent(BuildSystemEvent.DEPENDENCY_ANALYZER));
        this.loadDeclFileMap();
        const depAnalyzer = new DependencyAnalyzer(this.buildConfig, this.arktsConfigGenerator);
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
        const needRegenNodeIds = new Set<string>();
        for (const node of buildGraph.nodes) {
            const needsRegen = this.nodeNeedsRegeneration(node);
            const contentFiles: FileInfo[] = node.data.contentType === JobContentType.FILE
                ? [node.data.content as FileInfo]
                : node.data.content as FileInfo[];

            // Only include mappings for files in this job's fileList to reduce memory usage
            const jobFileToModuleMap = this.buildJobFileToModuleMap(contentFiles);
            const declgenJob: DeclgenV1JobInfo = {
                ...node.data,
                fileToModuleMap: jobFileToModuleMap
            };
            if (needsRegen) {
                declgenJobs.push(declgenJob);
                needRegenNodeIds.add(node.id);
            }

            newNodes.push({
                id: node.id,
                data: { ...declgenJob, buildConfig: this.buildConfig },
                predecessors: node.predecessors,
                descendants: node.descendants,
            });
        }

        await this.backupDeclgenFiles(declgenJobs);

        const newGraph: Graph<ProcessDeclgenV1Task> = Graph.createGraphFromNodes(newNodes);
        const filteredGraph = newGraph.filter((node: GraphNode<ProcessDeclgenV1Task>) => needRegenNodeIds.has(node.id));

        taskManager.buildGraph = filteredGraph;
        taskManager.initTaskQueue();

        // Ignore the result
        await taskManager.finish();

        await this.updateDeclFileMapForJobs(declgenJobs);
        await this.saveDeclFileMap();
    }
}
