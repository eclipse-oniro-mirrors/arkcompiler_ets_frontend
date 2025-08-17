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
import * as crypto from 'crypto';


import cluster, {
  Cluster,
  Worker,
} from 'cluster';
import { Worker as ThreadWorker } from 'worker_threads';
import {
  ABC_SUFFIX,
  ARKTSCONFIG_JSON_FILE,
  DEFAULT_WOKER_NUMS,
  DECL_ETS_SUFFIX,
  DECL_TS_SUFFIX,
  DEPENDENCY_INPUT_FILE,
  DEPENDENCY_JSON_FILE,
  LANGUAGE_VERSION,
  LINKER_INPUT_FILE,
  MERGED_ABC_FILE,
  MERGED_INTERMEDIATE_FILE,
  STATIC_RECORD_FILE,
  STATIC_RECORD_FILE_CONTENT,
  TS_SUFFIX
} from '../pre_define';
import {
  changeDeclgenFileExtension,
  changeFileExtension,
  createFileIfNotExists,
  ensurePathExists,
  getFileHash,
  isMac,
  isMixCompileProject
} from '../utils';
import {
  PluginDriver,
  PluginHook
} from '../plugins/plugins_driver';
import {
  Logger,
  LogData,
  LogDataFactory
} from '../logger';
import { ErrorCode } from '../error_code';
import {
  ArkTS,
  ArkTSGlobal,
  BuildConfig,
  BUILD_MODE,
  OHOS_MODULE_TYPE,
  CompileFileInfo,
  DependencyFileConfig,
  DependentModuleConfig,
  JobInfo,
  KPointer,
  ModuleInfo,
  ES2PANDA_MODE
} from '../types';
import {
  ArkTSConfig,
  ArkTSConfigGenerator
} from './generate_arktsconfig';
import { SetupClusterOptions } from '../types';
import { KitImportTransformer } from '../plugins/KitImportTransformer';

import { initKoalaModules } from '../init/init_koala_modules';

export abstract class BaseMode {
  public buildConfig: BuildConfig;
  public entryFiles: Set<string>;
  public allFiles: Map<string, CompileFileInfo>;
  public compileFiles: Map<string, CompileFileInfo>;
  public outputDir: string;
  public cacheDir: string;
  public pandaSdkPath: string;
  public buildSdkPath: string;
  public packageName: string;
  public sourceRoots: string[];
  public moduleRootPath: string;
  public moduleType: string;
  public dependentModuleList: DependentModuleConfig[];
  public moduleInfos: Map<string, ModuleInfo>;
  public mergedAbcFile: string;
  public dependencyJsonFile: string;
  public abcLinkerCmd: string[];
  public dependencyAnalyzerCmd: string[];
  public logger: Logger;
  public isDebug: boolean;
  public enableDeclgenEts2Ts: boolean;
  public declgenV1OutPath: string | undefined;
  public declgenV2OutPath: string | undefined;
  public declgenBridgeCodePath: string | undefined;
  public hasMainModule: boolean;
  public abcFiles: Set<string>;
  public hashCacheFile: string;
  public isCacheFileExists: boolean;
  public hashCache: Record<string, string>;
  public dependencyFileMap: DependencyFileConfig | null;
  public isBuildConfigModified: boolean | undefined;
  public hasCleanWorker: boolean;
  public byteCodeHar: boolean;
  public es2pandaMode: number;
  public skipDeclCheck: boolean;

  constructor(buildConfig: BuildConfig) {
    this.buildConfig = buildConfig;
    this.entryFiles = new Set<string>(buildConfig.compileFiles as string[]);
    this.allFiles = new Map<string, CompileFileInfo>();
    this.compileFiles = new Map<string, CompileFileInfo>();
    this.outputDir = buildConfig.loaderOutPath as string;
    this.cacheDir = buildConfig.cachePath as string;
    this.pandaSdkPath = buildConfig.pandaSdkPath as string;
    this.buildSdkPath = buildConfig.buildSdkPath as string;
    this.packageName = buildConfig.packageName as string;
    this.sourceRoots = buildConfig.sourceRoots as string[];
    this.moduleRootPath = buildConfig.moduleRootPath as string;
    this.moduleType = buildConfig.moduleType as string;
    this.dependentModuleList = buildConfig.dependentModuleList;
    this.moduleInfos = new Map<string, ModuleInfo>();
    this.mergedAbcFile = path.resolve(this.outputDir, MERGED_ABC_FILE);
    this.dependencyJsonFile = path.resolve(this.cacheDir, DEPENDENCY_JSON_FILE);
    this.abcLinkerCmd = ['"' + this.buildConfig.abcLinkerPath + '"'];
    this.dependencyAnalyzerCmd = ['"' + this.buildConfig.dependencyAnalyzerPath + '"'];
    this.logger = Logger.getInstance();
    this.isDebug = buildConfig.buildMode as string === BUILD_MODE.DEBUG;
    this.enableDeclgenEts2Ts = buildConfig.enableDeclgenEts2Ts as boolean;
    this.declgenV1OutPath = buildConfig.declgenV1OutPath as string | undefined;
    this.declgenV2OutPath = buildConfig.declgenV2OutPath as string | undefined;
    this.declgenBridgeCodePath = buildConfig.declgenBridgeCodePath as string | undefined;
    this.hasMainModule = buildConfig.hasMainModule;
    this.abcFiles = new Set<string>();
    this.hashCacheFile = path.join(this.cacheDir, 'hash_cache.json');
    this.isCacheFileExists = fs.existsSync(this.hashCacheFile);
    this.hashCache = this.loadHashCache();
    this.dependencyFileMap = null;
    this.isBuildConfigModified = buildConfig.isBuildConfigModified as boolean | undefined;
    this.hasCleanWorker = false;
    this.byteCodeHar = buildConfig.byteCodeHar as boolean;
    this.es2pandaMode = buildConfig?.es2pandaMode ?? (
      isMixCompileProject(buildConfig)
        ? ES2PANDA_MODE.RUN_PARALLEL
        : ES2PANDA_MODE.RUN
    );
    this.skipDeclCheck = buildConfig?.skipDeclCheck as boolean ?? true;
  }

  public declgen(fileInfo: CompileFileInfo): void {
    const source = fs.readFileSync(fileInfo.filePath, 'utf8');
    const moduleInfo: ModuleInfo = this.moduleInfos.get(fileInfo.packageName)!;
    const filePathFromModuleRoot: string = path.relative(moduleInfo.moduleRootPath, fileInfo.filePath);
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
    const arktsGlobal: ArkTSGlobal = this.buildConfig.arktsGlobal;
    const arkts: ArkTS = this.buildConfig.arkts;
    let errorStatus = false;
    try {
      const staticRecordPath = path.join(
        moduleInfo.declgenV1OutPath as string,
        STATIC_RECORD_FILE
      )
      const declEtsOutputDir = path.dirname(declEtsOutputPath);
      const staticRecordRelativePath = changeFileExtension(
        path.relative(declEtsOutputDir, staticRecordPath).replace(/\\/g, '\/'),
        "",
        DECL_TS_SUFFIX
      );
      createFileIfNotExists(staticRecordPath, STATIC_RECORD_FILE_CONTENT);

      arktsGlobal.filePath = fileInfo.filePath;
      arktsGlobal.config = arkts.Config.create([
        '_',
        '--extension',
        'ets',
        '--arktsconfig',
        fileInfo.arktsConfigFile,
        fileInfo.filePath
      ]).peer;
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
        staticRecordRelativePath
      ); // Generate 1.0 declaration files & 1.0 glue code
      this.logger.printInfo('declaration files generated');
    } catch (error) {
      errorStatus = true;
      if (error instanceof Error) {
        const logData: LogData = LogDataFactory.newInstance(
          ErrorCode.BUILDSYSTEM_DECLGEN_FAIL,
          'Generate declaration files failed.',
          error.message,
          fileInfo.filePath
        );
        this.logger.printError(logData);
      }
    } finally {
      if (!errorStatus) {
        // when error occur,wrapper will destroy context.
        arktsGlobal.es2panda._DestroyContext(arktsGlobal.compilerContext.peer);
      }
      arkts.destroyConfig(arktsGlobal.config);
    }
  }

  public compile(fileInfo: CompileFileInfo): void {
    ensurePathExists(fileInfo.abcFilePath);

    const ets2pandaCmd: string[] = [
      '_',
      '--extension',
      'ets',
      '--arktsconfig',
      fileInfo.arktsConfigFile,
      '--output',
      fileInfo.abcFilePath,
    ];

    if (this.isDebug) {
      ets2pandaCmd.push('--debug-info');
      ets2pandaCmd.push('--opt-level=0');
    }
    ets2pandaCmd.push(fileInfo.filePath);
    this.logger.printInfo('ets2pandaCmd: ' + ets2pandaCmd.join(' '));

    let { arkts, arktsGlobal } = initKoalaModules(this.buildConfig)
    let errorStatus = false;
    try {
      arktsGlobal.filePath = fileInfo.filePath;
      arktsGlobal.config = arkts.Config.create(ets2pandaCmd).peer;
      const source = fs.readFileSync(fileInfo.filePath).toString();
      arktsGlobal.compilerContext = arkts.Context.createFromString(source);
      PluginDriver.getInstance().getPluginContext().setArkTSProgram(arktsGlobal.compilerContext.program);

      arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_PARSED, arktsGlobal.compilerContext.peer);
      this.logger.printInfo('es2panda proceedToState parsed');
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
      PluginDriver.getInstance().runPluginHook(PluginHook.PARSED);
      this.logger.printInfo('plugin parsed finished');

      arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_CHECKED, arktsGlobal.compilerContext.peer);
      this.logger.printInfo('es2panda proceedToState checked');

      if (this.hasMainModule && (this.byteCodeHar || this.moduleType === OHOS_MODULE_TYPE.SHARED)) {
        let filePathFromModuleRoot: string = path.relative(this.moduleRootPath, fileInfo.filePath);
        let declEtsOutputPath: string = changeFileExtension(
          path.join(this.declgenV2OutPath as string, filePathFromModuleRoot),
          DECL_ETS_SUFFIX
        );
        ensurePathExists(declEtsOutputPath);

        // Generate 1.2 declaration files(a temporary solution while binary import not pushed)
        arkts.generateStaticDeclarationsFromContext(declEtsOutputPath);
      }

      ast = arkts.EtsScript.fromContext();
      PluginDriver.getInstance().getPluginContext().setArkTSAst(ast);
      PluginDriver.getInstance().runPluginHook(PluginHook.CHECKED);
      this.logger.printInfo('plugin checked finished');

      arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_BIN_GENERATED, arktsGlobal.compilerContext.peer);
      this.logger.printInfo('es2panda bin generated');
    } catch (error) {
      errorStatus = true;
      if (error instanceof Error) {
        const logData: LogData = LogDataFactory.newInstance(
          ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL,
          'Compile abc files failed.',
          error.message,
          fileInfo.filePath
        );
        this.logger.printError(logData);
      }
    } finally {
      if (!errorStatus) {
        // when error occur,wrapper will destroy context.
        arktsGlobal.es2panda._DestroyContext(arktsGlobal.compilerContext.peer);
      }
      PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
      arkts.destroyConfig(arktsGlobal.config);
    }
  }

  public compileMultiFiles(filePaths: string[], moduleInfo: ModuleInfo): void {
    const intermediateFilePath = path.resolve(this.cacheDir, MERGED_INTERMEDIATE_FILE);
    this.abcFiles.clear();
    this.abcFiles.add(intermediateFilePath);

    let ets2pandaCmd: string[] = [
      '_',
      '--extension',
      'ets',
      '--arktsconfig',
      moduleInfo.arktsConfigFile,
      '--output',
      intermediateFilePath,
      '--simultaneous'
    ];
    ensurePathExists(intermediateFilePath);
    if (this.isDebug) {
      ets2pandaCmd.push('--debug-info');
      ets2pandaCmd.push('--opt-level=0');
    }
    ets2pandaCmd.push(this.buildConfig.compileFiles[0]);
    this.logger.printInfo('ets2pandaCmd: ' + ets2pandaCmd.join(' '));

    let { arkts, arktsGlobal } = initKoalaModules(this.buildConfig);
    let errorStatus = false;
    try {
      arktsGlobal.config = arkts.Config.create(ets2pandaCmd).peer;
      //@ts-ignore
      arktsGlobal.compilerContext = arkts.Context.createContextGenerateAbcForExternalSourceFiles(this.buildConfig.compileFiles);;
      PluginDriver.getInstance().getPluginContext().setArkTSProgram(arktsGlobal.compilerContext.program);

      arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_PARSED, arktsGlobal.compilerContext.peer);
      this.logger.printInfo('es2panda proceedToState parsed');
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

      arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_CHECKED, arktsGlobal.compilerContext.peer);
      this.logger.printInfo('es2panda proceedToState checked');

      if (this.hasMainModule && (this.byteCodeHar || this.moduleType === OHOS_MODULE_TYPE.SHARED)) {
        for (const sourceFilePath of this.buildConfig.compileFiles) {
          const filePathFromModuleRoot: string = path.relative(this.moduleRootPath, sourceFilePath);

          const declEtsOutputPath: string = changeFileExtension(
            path.join(this.declgenV2OutPath as string, filePathFromModuleRoot),
            DECL_ETS_SUFFIX
          );
          ensurePathExists(declEtsOutputPath);

          arkts.generateStaticDeclarationsFromContext(declEtsOutputPath);
        }
      }

      ast = arkts.EtsScript.fromContext();
      PluginDriver.getInstance().getPluginContext().setArkTSAst(ast);
      PluginDriver.getInstance().runPluginHook(PluginHook.CHECKED);
      this.logger.printInfo('plugin checked finished');

      arkts.proceedToState(arkts.Es2pandaContextState.ES2PANDA_STATE_BIN_GENERATED, arktsGlobal.compilerContext.peer);
      this.logger.printInfo('es2panda bin generated');
    } catch (error) {
      errorStatus = true;
      throw error;
    } finally {
      if (!errorStatus) {
        // when error occur,wrapper will destroy context.
        arktsGlobal.es2panda._DestroyContext(arktsGlobal.compilerContext.peer);
      }
      PluginDriver.getInstance().runPluginHook(PluginHook.CLEAN);
      arkts.destroyConfig(arktsGlobal.config);
    }
  }

  public mergeAbcFiles(): void {
    let linkerInputFile: string = path.join(this.cacheDir, LINKER_INPUT_FILE);
    let linkerInputContent: string = '';
    this.abcFiles.forEach((abcFile: string) => {
      linkerInputContent += abcFile + os.EOL;
    });
    fs.writeFileSync(linkerInputFile, linkerInputContent);

    this.abcLinkerCmd.push('--output');
    this.abcLinkerCmd.push('"' + this.mergedAbcFile + '"');
    this.abcLinkerCmd.push('--');
    this.abcLinkerCmd.push('@' + '"' + linkerInputFile + '"');

    let abcLinkerCmdStr: string = this.abcLinkerCmd.join(' ');
    if (isMac()) {
      const loadLibrary = 'DYLD_LIBRARY_PATH=' + '"' + process.env.DYLD_LIBRARY_PATH + '"';
      abcLinkerCmdStr = loadLibrary + ' ' + abcLinkerCmdStr;
    }
    this.logger.printInfo(abcLinkerCmdStr);

    ensurePathExists(this.mergedAbcFile);
    try {
      child_process.execSync(abcLinkerCmdStr).toString();
    } catch (error) {
      if (error instanceof Error) {
        const logData: LogData = LogDataFactory.newInstance(
          ErrorCode.BUILDSYSTEM_LINK_ABC_FAIL,
          'Link abc files failed.',
          error.message
        );
        this.logger.printError(logData);
      }
    }
  }

  private getDependentModules(moduleInfo: ModuleInfo): Map<string, ModuleInfo>[] {
    const dynamicDepModules: Map<string, ModuleInfo> = new Map<string, ModuleInfo>();
    const staticDepModules: Map<string, ModuleInfo> = new Map<string, ModuleInfo>();
    this.collectDependencyModules(moduleInfo.packageName, moduleInfo, dynamicDepModules, staticDepModules);

    if (moduleInfo.dependencies) {
      moduleInfo.dependencies.forEach((packageName: string) => {
        let depModuleInfo: ModuleInfo | undefined = this.moduleInfos.get(packageName);
        if (!depModuleInfo) {
          const logData: LogData = LogDataFactory.newInstance(
            ErrorCode.BUILDSYSTEM_DEPENDENT_MODULE_INFO_NOT_FOUND,
            `Module ${packageName} not found in moduleInfos`
          );
          this.logger.printErrorAndExit(logData);
        } else {
          this.collectDependencyModules(packageName, depModuleInfo, dynamicDepModules, staticDepModules);
        }
      });
    }
    return [dynamicDepModules, staticDepModules];
  }

  private collectDependencyModules(
    packageName: string,
    module: ModuleInfo,
    dynamicDepModules: Map<string, ModuleInfo>,
    staticDepModules: Map<string, ModuleInfo>
  ): void {
    if (module.language === LANGUAGE_VERSION.ARKTS_1_2) {
      staticDepModules.set(packageName, module);
    } else if (module.language === LANGUAGE_VERSION.ARKTS_1_1) {
      dynamicDepModules.set(packageName, module);
    } else if (module.language === LANGUAGE_VERSION.ARKTS_HYBRID) {
      staticDepModules.set(packageName, module);
      dynamicDepModules.set(packageName, module);
    }
  }

  protected generateArkTSConfigForModules(): void {
    let taskList: ModuleInfo[] = [];
    this.moduleInfos.forEach((moduleInfo: ModuleInfo, moduleRootPath: string) => {
      if (moduleInfo.dependenciesSet.size === 0) {
        taskList.push(moduleInfo);
      }

      ArkTSConfigGenerator.getInstance(this.buildConfig, this.moduleInfos)
        .generateArkTSConfigFile(moduleInfo, this.enableDeclgenEts2Ts);
    });

    while (taskList.length > 0) {
      const task = taskList.pop();
      const arktsConfig = ArkTSConfigGenerator.getInstance().getArktsConfigPackageName(task!!.packageName)
      task?.dependencies?.forEach(dependecyModule => {
        arktsConfig?.mergeArktsConfig(
          ArkTSConfigGenerator.getInstance().getArktsConfigPackageName(dependecyModule)
        );
      });
      fs.writeFileSync(task!!.arktsConfigFile, JSON.stringify(arktsConfig!!.getCompilerOptions(), null, 2))
      task?.dependentSet.forEach((dependentTask) => {
        const dependentModule = this.moduleInfos.get(dependentTask);
        dependentModule?.dependenciesSet.delete(task.packageName);
        if (dependentModule?.dependenciesSet.size === 0) {
          taskList.push(dependentModule);
        }
      });
    }
  }

  private collectDepModuleInfos(): void {
    this.moduleInfos.forEach((moduleInfo: ModuleInfo) => {
      let [dynamicDepModules, staticDepModules] = this.getDependentModules(moduleInfo);
      moduleInfo.dynamicDepModuleInfos = dynamicDepModules;
      moduleInfo.staticDepModuleInfos = staticDepModules;

      [...dynamicDepModules.keys(), ...staticDepModules.keys()].forEach(depName => {
        moduleInfo.dependenciesSet.add(depName);
      });
      moduleInfo.dependenciesSet.delete(moduleInfo.packageName);
      moduleInfo.dependencies?.forEach(moduleName => {
        this.moduleInfos.get(moduleName)?.dependentSet.add(moduleInfo.packageName);
      });
    });
  }

  protected collectModuleInfos(): void {
    if (this.hasMainModule && (!this.packageName || !this.moduleRootPath || !this.sourceRoots)) {
      const logData: LogData = LogDataFactory.newInstance(
        ErrorCode.BUILDSYSTEM_MODULE_INFO_NOT_CORRECT_FAIL,
        'Main module info from hvigor is not correct.'
      );
      this.logger.printError(logData);
    }
    const mainModuleInfo: ModuleInfo = this.getMainModuleInfo();
    this.moduleInfos.set(this.packageName, mainModuleInfo);
    this.dependentModuleList.forEach((module: DependentModuleConfig) => {
      if (!module.packageName || !module.modulePath || !module.sourceRoots || !module.entryFile) {
        const logData: LogData = LogDataFactory.newInstance(
          ErrorCode.BUILDSYSTEM_DEPENDENT_MODULE_INFO_NOT_CORRECT_FAIL,
          'Dependent module info from hvigor is not correct.'
        );
        this.logger.printError(logData);
      }
      if (this.moduleInfos.has(module.packageName)) {
        return;
      }
      let moduleInfo: ModuleInfo = {
        isMainModule: false,
        packageName: module.packageName,
        moduleRootPath: module.modulePath,
        moduleType: module.moduleType,
        sourceRoots: module.sourceRoots,
        entryFile: module.entryFile,
        arktsConfigFile: path.resolve(this.cacheDir, module.packageName, ARKTSCONFIG_JSON_FILE),
        compileFileInfos: [],
        dynamicDepModuleInfos: new Map<string, ModuleInfo>(),
        staticDepModuleInfos: new Map<string, ModuleInfo>(),
        declgenV1OutPath: module.declgenV1OutPath,
        declgenV2OutPath: module.declgenV2OutPath,
        declgenBridgeCodePath: module.declgenBridgeCodePath,
        language: module.language,
        declFilesPath: module.declFilesPath,
        dependencies: module.dependencies,
        byteCodeHar: module.byteCodeHar,
        abcPath: module.abcPath,
        dependenciesSet: new Set(module?.dependencies),
        dependentSet: new Set(),
      };
      this.moduleInfos.set(module.packageName, moduleInfo);
    });
    this.collectDepModuleInfos();
  }

  protected getMainModuleInfo(): ModuleInfo {
    const mainModuleInfo = this.dependentModuleList.find((module: DependentModuleConfig) => module.packageName === this.packageName);
    return {
      isMainModule: this.hasMainModule,
      packageName: mainModuleInfo?.packageName ?? this.packageName,
      moduleRootPath: mainModuleInfo?.modulePath ?? this.moduleRootPath,
      moduleType: mainModuleInfo?.moduleType ?? this.moduleType,
      sourceRoots: this.sourceRoots,
      entryFile: '',
      arktsConfigFile: path.resolve(this.cacheDir, this.packageName, ARKTSCONFIG_JSON_FILE),
      dynamicDepModuleInfos: new Map<string, ModuleInfo>(),
      staticDepModuleInfos: new Map<string, ModuleInfo>(),
      compileFileInfos: [],
      declgenV1OutPath: mainModuleInfo?.declgenV1OutPath ?? this.declgenV1OutPath,
      declgenV2OutPath: mainModuleInfo?.declgenV2OutPath ?? this.declgenV2OutPath,
      declgenBridgeCodePath: mainModuleInfo?.declgenBridgeCodePath ?? this.declgenBridgeCodePath,
      byteCodeHar: this.byteCodeHar,
      language: mainModuleInfo?.language ?? LANGUAGE_VERSION.ARKTS_1_2,
      declFilesPath: mainModuleInfo?.declFilesPath,
      dependentSet: new Set(),
      dependenciesSet: new Set(mainModuleInfo?.dependencies),
      dependencies: mainModuleInfo?.dependencies ?? []
    };
  }

  private loadHashCache(): Record<string, string> {
    try {
      if (!fs.existsSync(this.hashCacheFile)) {
        return {};
      }

      const cacheContent: string = fs.readFileSync(this.hashCacheFile, 'utf-8');
      const cacheData: Record<string, string> = JSON.parse(cacheContent);
      const filteredCache: Record<string, string> = Object.fromEntries(
        Object.entries(cacheData).filter(([file]) => this.entryFiles.has(file))
      );
      return filteredCache;
    } catch (error) {
      if (error instanceof Error) {
        const logData: LogData = LogDataFactory.newInstance(
          ErrorCode.BUILDSYSTEM_LOAD_HASH_CACHE_FAIL,
          'Failed to load hash cache.',
          error.message
        );
        this.logger.printError(logData);
      }
      return {};
    }
  }

  private saveHashCache(): void {
    ensurePathExists(this.hashCacheFile);
    fs.writeFileSync(this.hashCacheFile, JSON.stringify(this.hashCache, null, 2));
  }

  private isFileChanged(etsFilePath: string, abcFilePath: string): boolean {
    if (fs.existsSync(abcFilePath)) {
      const etsFileLastModified: number = fs.statSync(etsFilePath).mtimeMs;
      const abcFileLastModified: number = fs.statSync(abcFilePath).mtimeMs;
      if (etsFileLastModified < abcFileLastModified) {
        const currentHash = getFileHash(etsFilePath);
        const cachedHash = this.hashCache[etsFilePath];
        if (cachedHash && cachedHash === currentHash) {
          return false;
        }
      }
    }
    return true;
  }

  private collectDependentCompileFiles(): void {
    if (!this.dependencyFileMap) {
      const logData: LogData = LogDataFactory.newInstance(
        ErrorCode.BUILDSYSTEM_Dependency_Analyze_FAIL,
        'Analyze files dependency failed.',
        'Dependency map not initialized.'
      );
      this.logger.printError(logData);
      return;
    }

    const compileFiles = new Set<string>();
    const processed = new Set<string>();
    const queue: string[] = [];

    this.entryFiles.forEach((file: string) => {
      // Skip the declaration files when compiling abc
      if (file.endsWith(DECL_ETS_SUFFIX)) {
        return;
      }
      let hasModule = false;
      for (const [_, moduleInfo] of this.moduleInfos) {
        if (!file.startsWith(moduleInfo.moduleRootPath)) {
          continue;
        }

        hasModule = true;
        const filePathFromModuleRoot = path.relative(moduleInfo.moduleRootPath, file);
        const filePathInCache = path.join(this.cacheDir, moduleInfo.packageName, filePathFromModuleRoot);
        const abcFilePath = path.resolve(changeFileExtension(filePathInCache, ABC_SUFFIX));
        this.abcFiles.add(abcFilePath);

        const fileInfo: CompileFileInfo = {
          filePath: file,
          dependentFiles: this.dependencyFileMap?.dependants[file] || [],
          abcFilePath,
          arktsConfigFile: moduleInfo.arktsConfigFile,
          packageName: moduleInfo.packageName
        };
        this.allFiles.set(file, fileInfo);

        if (this.isBuildConfigModified || this.isFileChanged(file, abcFilePath)) {
          compileFiles.add(file);
          queue.push(file);
        }
        this.hashCache[file] = getFileHash(file);
        break;
      }
      if (!hasModule) {
        const logData: LogData = LogDataFactory.newInstance(
          ErrorCode.BUILDSYSTEM_FILE_NOT_BELONG_TO_ANY_MODULE_FAIL,
          'File does not belong to any module in moduleInfos.',
          '',
          file
        );
        this.logger.printError(logData);
        return;
      }
    });
    this.collectAbcFileFromByteCodeHar();

    while (queue.length > 0) {
      const currentFile = queue.shift()!;
      processed.add(currentFile);

      (this.dependencyFileMap?.dependants[currentFile] || []).forEach(dependant => {
        // For the 1.1 declaration file referenced in dependencies, if a path is detected as non-existent, it will be skipped.
        const isFileExist = fs.existsSync(dependant);
        if (!isFileExist) {
          return;
        }
        if (!compileFiles.has(dependant) && !processed.has(dependant)) {
          queue.push(dependant);
        }
        compileFiles.add(dependant);
      });
    }

    compileFiles.forEach((file: string) => {
      let hasModule = false;
      for (const [_, moduleInfo] of this.moduleInfos) {
        if (!file.startsWith(moduleInfo.moduleRootPath)) {
          continue;
        }
        hasModule = true;
        const filePathFromModuleRoot = path.relative(moduleInfo.moduleRootPath, file);
        const filePathInCache = path.join(this.cacheDir, moduleInfo.packageName, filePathFromModuleRoot);
        const abcFilePath = path.resolve(changeFileExtension(filePathInCache, ABC_SUFFIX));

        const fileInfo: CompileFileInfo = {
          filePath: file,
          dependentFiles: this.dependencyFileMap?.dependants[file] || [],
          abcFilePath,
          arktsConfigFile: moduleInfo.arktsConfigFile,
          packageName: moduleInfo.packageName
        };

        moduleInfo.compileFileInfos.push(fileInfo);
        this.compileFiles.set(file, fileInfo);
        break;
      }
      if (!hasModule) {
        const logData: LogData = LogDataFactory.newInstance(
          ErrorCode.BUILDSYSTEM_FILE_NOT_BELONG_TO_ANY_MODULE_FAIL,
          'File does not belong to any module in moduleInfos.',
          '',
          file
        );
        this.logger.printError(logData);
      }
    });
  }

  private shouldSkipFile(file: string, moduleInfo: ModuleInfo, filePathFromModuleRoot: string, abcFilePath: string): boolean {
    const targetPath = this.enableDeclgenEts2Ts
      ? changeFileExtension(path.join(moduleInfo.declgenBridgeCodePath as string, moduleInfo.packageName, filePathFromModuleRoot), TS_SUFFIX)
      : abcFilePath;
    return !this.isFileChanged(file, targetPath);
  }

  protected collectCompileFiles(): void {
    this.entryFiles.forEach((file: string) => {
      for (const [packageName, moduleInfo] of this.moduleInfos) {
        const relativePath = path.relative(moduleInfo.moduleRootPath, file);
        if (relativePath.startsWith('..') || path.isAbsolute(relativePath)) {
          continue;
        }
        const filePathFromModuleRoot: string = path.relative(moduleInfo.moduleRootPath, file);
        const filePathInCache: string = path.join(this.cacheDir, moduleInfo.packageName, filePathFromModuleRoot);
        const abcFilePath: string = path.resolve(changeFileExtension(filePathInCache, ABC_SUFFIX));
        this.abcFiles.add(abcFilePath);
        this.hashCache[file] = getFileHash(file);
        const fileInfo: CompileFileInfo = {
          filePath: path.resolve(file),
          dependentFiles: [],
          abcFilePath: abcFilePath,
          arktsConfigFile: moduleInfo.arktsConfigFile,
          packageName: moduleInfo.packageName
        };
        moduleInfo.compileFileInfos.push(fileInfo);
        this.compileFiles.set(path.resolve(file), fileInfo);
        return;
      }
      const logData: LogData = LogDataFactory.newInstance(
        ErrorCode.BUILDSYSTEM_FILE_NOT_BELONG_TO_ANY_MODULE_FAIL,
        'File does not belong to any module in moduleInfos.',
        '',
        file
      );
      this.logger.printError(logData);
    });
  }

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

  protected generateModuleInfos(): void {
    this.collectModuleInfos();
    this.generateArkTSConfigForModules();
    this.collectCompileFiles();
    this.saveHashCache();
  }

  public async generateDeclaration(): Promise<void> {
    this.generateModuleInfos();

    const compilePromises: Promise<void>[] = [];
    this.compileFiles.forEach((fileInfo: CompileFileInfo, _: string) => {
      compilePromises.push(new Promise<void>((resolve) => {
        this.declgen(fileInfo);
        resolve();
      }));
    });
    await Promise.all(compilePromises);
  }

  public async run(): Promise<void> {
    this.generateModuleInfos();

    const compilePromises: Promise<void>[] = [];
    let moduleToFile = new Map<string, string[]>();
    this.compileFiles.forEach((fileInfo: CompileFileInfo, file: string) => {
      if (!moduleToFile.has(fileInfo.packageName)) {
        moduleToFile.set(fileInfo.packageName, []);
      }
      moduleToFile.get(fileInfo.packageName)?.push(fileInfo.filePath);
    });
    try {
      //@ts-ignore
      this.compileMultiFiles([], this.moduleInfos.get(this.packageName));
    } catch (error) {
      if (error instanceof Error) {
        const logData: LogData = LogDataFactory.newInstance(
          ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL,
          'Compile abc files failed.',
          error.message
        );
        this.logger.printErrorAndExit(logData);
      }
    }
    
    this.mergeAbcFiles();
  }

  // -- runParallell code begins --
  private terminateAllWorkers(): void {
    Object.values(cluster.workers || {}).forEach(worker => {
      worker?.kill();
    });
  };

  public generatedependencyFileMap(): void {
    if (this.enableDeclgenEts2Ts) {
      return;
    }
    const dependencyInputFile: string = path.join(this.cacheDir, DEPENDENCY_INPUT_FILE);
    let dependencyInputContent: string = '';
    this.entryFiles.forEach((entryFile: string) => {
      dependencyInputContent += entryFile + os.EOL;
    });
    fs.writeFileSync(dependencyInputFile, dependencyInputContent);

    this.dependencyAnalyzerCmd.push('@' + '"' + dependencyInputFile + '"');
    for (const [_, module] of this.moduleInfos) {
      if (module.isMainModule) {
        this.dependencyAnalyzerCmd.push('--arktsconfig=' + '"' + module.arktsConfigFile + '"');
        break;
      }
    }
    this.dependencyAnalyzerCmd.push('--output=' + '"' + this.dependencyJsonFile + '"');
    let dependencyAnalyzerCmdStr: string = this.dependencyAnalyzerCmd.join(' ');
    if (isMac()) {
      const loadLibrary = 'DYLD_LIBRARY_PATH=' + '"' + process.env.DYLD_LIBRARY_PATH + '"';
      dependencyAnalyzerCmdStr = loadLibrary + ' ' + dependencyAnalyzerCmdStr;
    }
    this.logger.printInfo(dependencyAnalyzerCmdStr);

    ensurePathExists(this.dependencyJsonFile);
    try {
      const output = child_process.execSync(dependencyAnalyzerCmdStr, {
        stdio: 'pipe',
        encoding: 'utf-8'
      });
      if (output.trim() !== '') {
        const logData: LogData = LogDataFactory.newInstance(
          ErrorCode.BUILDSYSTEM_Dependency_Analyze_FAIL,
          'Analyze files dependency failed.',
          output
        );
        this.logger.printError(logData);
        return;
      }
      const dependencyJsonContent = fs.readFileSync(this.dependencyJsonFile, 'utf-8');
      this.dependencyFileMap = JSON.parse(dependencyJsonContent);
    } catch (error) {
      if (error instanceof Error) {
        const execError = error as child_process.ExecException;
        let fullErrorMessage = execError.message;
        if (execError.stderr) {
          fullErrorMessage += `\nError output: ${execError.stderr}`;
        }
        if (execError.stdout) {
          fullErrorMessage += `\nOutput: ${execError.stdout}`;
        }
        const logData: LogData = LogDataFactory.newInstance(
          ErrorCode.BUILDSYSTEM_Dependency_Analyze_FAIL,
          'Analyze files dependency failed.',
          fullErrorMessage
        );
        this.logger.printError(logData);
      }
    }
  }

  public async runParallel(): Promise<void> {
    this.generateModuleInfos();

    const isPrimary = cluster.isPrimary ?? cluster.isMaster; // Adapt to node-v14
    if (!isPrimary) {
      return;
    }

    try {
      this.setupCluster(cluster, {
        clearExitListeners: true,
        execPath: path.resolve(__dirname, 'compile_worker.js'),
      });
      await this.dispatchTasks();
      this.logger.printInfo('All tasks complete, merging...');
      this.mergeAbcFiles();
    } catch (error) {
      this.logger.printError(LogDataFactory.newInstance(
        ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL,
        'Compile abc files failed.'
      ));
    } finally {
      this.terminateAllWorkers();
    }
  }

  public async generateDeclarationParallell(): Promise<void> {
    this.generateModuleInfos();
    this.generateArkTSConfigForModules();

    const isPrimary = cluster.isPrimary ?? cluster.isMaster;
    if (!isPrimary) {
      return;
    }

    try {
      this.setupCluster(cluster, {
        clearExitListeners: true,
        execPath: path.resolve(__dirname, 'declgen_worker.js'),
      });
      await this.dispatchTasks();
      this.logger.printInfo('All declaration generation tasks complete.');
    } catch (error) {
      this.logger.printError(LogDataFactory.newInstance(
        ErrorCode.BUILDSYSTEM_DECLGEN_FAIL,
        'Generate declaration files failed.'
      ));
    } finally {
      this.terminateAllWorkers();
    }
  }

  private async dispatchTasks(): Promise<void> {
    const numCPUs = os.cpus().length;
    const taskQueue = Array.from(this.compileFiles.values());

    const configuredWorkers = this.buildConfig?.maxWorkers;
    const defaultWorkers = DEFAULT_WOKER_NUMS;

    let effectiveWorkers: number;

    if (configuredWorkers) {
      effectiveWorkers = Math.min(configuredWorkers, numCPUs - 1);
    } else {
      effectiveWorkers = Math.min(defaultWorkers, numCPUs - 1);
    }

    const maxWorkers = Math.min(taskQueue.length, effectiveWorkers);

    const chunkSize = Math.ceil(taskQueue.length / maxWorkers);
    const serializableConfig = this.getSerializableConfig();
    const workerExitPromises: Promise<void>[] = [];

    const moduleInfosArray = Array.from(this.moduleInfos.entries());

    for (let i = 0; i < maxWorkers; i++) {
      const taskChunk = taskQueue.slice(i * chunkSize, (i + 1) * chunkSize);
      const worker = cluster.fork();

      this.setupWorkerMessageHandler(worker);
      worker.send({ taskList: taskChunk, buildConfig: serializableConfig, moduleInfos: moduleInfosArray });

      const exitPromise = new Promise<void>((resolve, reject) => {
        worker.on('exit', (status) => status === 0 ? resolve() : reject());
      });

      workerExitPromises.push(exitPromise);
    }

    await Promise.all(workerExitPromises);
  }

  private setupWorkerMessageHandler(worker: Worker): void {
    worker.on('message', (message: {
      success: boolean;
      filePath?: string;
      error?: string;
      isDeclFile?: boolean;
    }) => {
      if (message.success) {
        return;
      }
      if (message.isDeclFile) {
        this.logger.printError(LogDataFactory.newInstance(
          ErrorCode.BUILDSYSTEM_DECLGEN_FAIL,
          'Generate declaration files failed in worker.',
          message.error || 'Unknown error',
          message.filePath
        ));
        return;
      }
      this.logger.printError(LogDataFactory.newInstance(
        ErrorCode.BUILDSYSTEM_COMPILE_ABC_FAIL,
        'Compile abc files failed in worker.',
        message.error || 'Unknown error',
        message.filePath
      ));
    });
  }

  private getSerializableConfig(): Object {
    const ignoreList = [
      'arkts',
    ];
    const jsonStr = JSON.stringify(this.buildConfig, (key, value) => {
      if (typeof value === 'bigint') {
        return undefined;
      }
      //remove useless data from buildConfig
      if (ignoreList.includes(key)) {
        return undefined;
      }
      return value;
    });
    return JSON.parse(jsonStr);
  }

  setupCluster(cluster: Cluster, options: SetupClusterOptions): void {
    const {
      clearExitListeners,
      execPath,
      execArgs = [],
    } = options;

    if (clearExitListeners) {
      cluster.removeAllListeners('exit');
    }
    const setupFn = cluster.setupPrimary ?? cluster.setupMaster; // Adapt to node-v14
    setupFn({
      exec: execPath,
      execArgv: execArgs,
    });
  }
  // -- runParallell code ends --


  // -- runConcurrent code begins --

  private findStronglyConnectedComponents(graph: DependencyFileConfig): Map<string, Set<string>> {
    const adjacencyList: Record<string, string[]> = {};
    const reverseAdjacencyList: Record<string, string[]> = {};
    const allNodes = new Set<string>();

    for (const node in graph.dependencies) {
      allNodes.add(node);
      graph.dependencies[node].forEach(dep => allNodes.add(dep));
    }
    for (const node in graph.dependants) {
      allNodes.add(node);
      graph.dependants[node].forEach(dep => allNodes.add(dep));
    }

    Array.from(allNodes).forEach(node => {
      adjacencyList[node] = graph.dependencies[node] || [];
      reverseAdjacencyList[node] = graph.dependants[node] || [];
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

    Array.from(allNodes).forEach(node => {
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
          const hashKey = createHash(sortedFiles.join('|'));
          components.set(hashKey, component);
        }

      }
    }

    return components;
  }


  private getJobDependencies(fileDeps: string[], cycleFiles: Map<string, string[]>): Set<string> {
    let depJobList: Set<string> = new Set<string>();
    fileDeps.forEach((file) => {
      if (!cycleFiles.has(file)) {
        depJobList.add(this.getExternalProgramJobId(file));
      } else {
        cycleFiles.get(file)?.forEach((f) => {
          depJobList.add(f);
        });
      }
    });

    return depJobList;
  }

  private getAbcJobId(file: string): string {
    return '1' + file;
  }

  private getExternalProgramJobId(file: string): string {
    return '0' + file;
  }

  private getJobDependants(fileDeps: string[], cycleFiles: Map<string, string[]>): Set<string> {
    let depJobList: Set<string> = new Set<string>();
    fileDeps.forEach((file) => {
      if (!file.endsWith(DECL_ETS_SUFFIX)) {
        depJobList.add(this.getAbcJobId(file));
      }
      if (cycleFiles.has(file)) {
        cycleFiles.get(file)?.forEach((f) => {
          depJobList.add(f);
        });
      } else {
        depJobList.add(this.getExternalProgramJobId(file));
      }
    });

    return depJobList;
  }

  private collectCompileJobs(jobs: Record<string, Job>): void {
    let fileDepsInfo: DependencyFileConfig = this.dependencyFileMap!;
    Object.keys(fileDepsInfo.dependants).forEach((file) => {
      if (!(file in fileDepsInfo.dependencies)) {
        fileDepsInfo.dependencies[file] = [];
      }
    });

    const cycleGroups = this.findStronglyConnectedComponents(fileDepsInfo);
    let cycleFiles: Map<string, string[]> = new Map<string, string[]>();
    cycleGroups.forEach((value: Set<string>, key: string) => {
      value.forEach((file) => {
        cycleFiles.set(file, [key]);
      });
    });

    Object.entries(fileDepsInfo.dependencies).forEach(([key, value]) => {
      if (this.entryFiles.has(key) && !this.compileFiles.has(key)) {
        return;
      }
      let dependencies = this.getJobDependencies(value, cycleFiles);

      if (!key.endsWith(DECL_ETS_SUFFIX)) {
        let abcJobId: string = this.getAbcJobId(key);
        jobs[abcJobId] = {
          id: abcJobId,
          isDeclFile: false,
          isInCycle: cycleFiles.has(key),
          isAbcJob: true,
          fileList: [key],
          dependencies: Array.from(dependencies), // 依赖external program
          dependants: []
        };
      }

      if (cycleFiles.has(key)) {
        const externalProgramJobIds = cycleFiles.get(key)!;
        externalProgramJobIds.forEach((id) => {
          let fileList: string[] = Array.from(cycleGroups.get(id)!);
          this.createExternalProgramJob(id, fileList, jobs, dependencies, true);
        });
      } else {
        const id = this.getExternalProgramJobId(key);
        let fileList: string[] = [key];
        this.createExternalProgramJob(id, fileList, jobs, dependencies);
      }

      if (key.endsWith(DECL_ETS_SUFFIX)) {
        let fileInfo: CompileFileInfo = {
          filePath: key,
          dependentFiles: [],
          abcFilePath: '',
          arktsConfigFile: this.moduleInfos.get(this.packageName)!.arktsConfigFile,
          packageName: this.moduleInfos.get(this.packageName)!.packageName
        };

        if (!this.allFiles.has(key)) {
          this.allFiles.set(key, fileInfo);
        }
      }
    });

    Object.entries(fileDepsInfo.dependants).forEach(([key, value]) => {
      if (this.entryFiles.has(key) && !this.compileFiles.has(key)) {
        return;
      }
      let dependants = this.getJobDependants(value, cycleFiles);

      this.dealWithDependants(cycleFiles, key, jobs, dependants);
    });
  }

  private dealWithDependants(cycleFiles: Map<string, string[]>, key: string, jobs: Record<string, Job>, dependants: Set<string>): void {
    if (cycleFiles.has(key)) {
      const externalProgramJobIds = cycleFiles.get(key)!;
      externalProgramJobIds.forEach((id) => {
        jobs[id].dependants.forEach(dep => {
          dependants.add(dep);
        });
        if (dependants.has(id)) {
          dependants.delete(id);
        }

        jobs[id].dependants = Array.from(dependants);
      });
    } else {
      const id = this.getExternalProgramJobId(key);
      jobs[id].dependants.forEach(dep => {
        dependants.add(dep);
      });
      if (dependants.has(id)) {
        dependants.delete(id);
      }
      jobs[id].dependants = Array.from(dependants);
    }
  }

  private createExternalProgramJob(id: string, fileList: string[], jobs: Record<string, Job>, dependencies: Set<string>, isInCycle?: boolean): void {
    if (dependencies.has(id)) {
      dependencies.delete(id);
    }

    // TODO: can be duplicated ids
    if (jobs[id]) {
      // If job already exists, merge the file lists and dependencies
      const existingJob = jobs[id];
      const mergedDependencies = new Set([
        ...existingJob.dependencies,
        ...Array.from(dependencies)
      ]);
      jobs[id] = {
        ...existingJob,
        dependencies: Array.from(mergedDependencies)
      };
    } else {
      jobs[id] = {
        id,
        fileList,
        isDeclFile: true,
        isInCycle,
        isAbcJob: false,
        dependencies: Array.from(dependencies), // 依赖external program
        dependants: []
      };
    }
  }

  private addJobToQueues(job: Job, queues: Queues): void {
    if (queues.externalProgramQueue.some(j => j.id === job.id) ||
      queues.abcQueue.some(j => j.id === job.id)) {
      return;
    }

    if (!job.isAbcJob) {
      queues.externalProgramQueue.push(job);
    } else {
      queues.abcQueue.push(job);
    }
  }

  private initCompileQueues(jobs: Record<string, Job>, queues: Queues): void {
    this.collectCompileJobs(jobs);
    Object.values(jobs).forEach(job => {
      if (job.dependencies.length === 0) {
        this.addJobToQueues(job, queues);
      }
    });
  }

  private checkAllTasksDone(queues: Queues, workerPool: WorkerInfo[]): boolean {
    if (queues.externalProgramQueue.length === 0) {
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

    if (this.hasCleanWorker) {
      return;
    }
    this.hasCleanWorker = true;
    let arktsGlobal = this.buildConfig.arktsGlobal;
    let arkts = this.buildConfig.arkts;

    arktsGlobal.es2panda._DestroyGlobalContext(globalContext);
    arkts.destroyConfig(config);
    arktsGlobal.es2panda._MemFinalize();

    this.mergeAbcFiles();
  }

  // CC-OFFNXT(huge_depth)
  private async invokeWorkers(jobs: Record<string, Job>, queues: Queues, processingJobs: Set<string>, workers: ThreadWorker[]): Promise<void> {
    return new Promise<void>((resolve) => {
      const numWorkers = 1;

      let files: string[] = [];

      Object.entries(jobs).forEach(([key, job]) => {
        for (let i = 0; i < job.fileList.length; i++) {
          files.push(job.fileList[i]);
        }
      });

      let arkts = this.buildConfig.arkts;
      let fileInfo = this.compileFiles.values().next().value!;

      let ets2pandaCmd: string[] = [
        '_',
        '--extension',
        'ets',
        '--arktsconfig',
        fileInfo.arktsConfigFile,
        '--output',
        fileInfo.abcFilePath,
      ];

      if (this.isDebug) {
        ets2pandaCmd.push('--debug-info');
        ets2pandaCmd.push('--opt-level=0');
      }
      ets2pandaCmd.push(fileInfo.filePath);

      arkts.MemInitialize();

      let config = arkts.Config.create(ets2pandaCmd).peer;

      let globalContextPtr = arkts.CreateGlobalContext(config, files, files.length, false);
      const serializableConfig = this.getSerializableConfig();

      const workerPool: WorkerInfo[] = [];
      for (let i = 0; i < numWorkers; i++) {
        const worker = new ThreadWorker(
          path.resolve(__dirname, 'compile_thread_worker.js'),
          { workerData: { workerId: i } }
        );

        workers.push(worker);
        workerPool.push({ worker, isIdle: true });
        this.assignTaskToIdleWorker(workerPool[i], queues, processingJobs, serializableConfig, globalContextPtr);
        worker.on('message', (msg) => {
          if (msg.type === 'TASK_FINISH') {
            const workerInfo = workerPool.find(w => w.worker === worker);
            if (workerInfo) {
              workerInfo.isIdle = true;
            }
            const jobId = msg.jobId;
            finishedJob.push(jobId);
            processingJobs.delete(jobId);
            const completedJob = jobs[jobId];
            completedJob.dependants.forEach(depJobId => {
              const depJob = jobs[depJobId];
              if (!depJob) {
                return;
              }
              const depIndex = depJob.dependencies.indexOf(jobId);
              if (depIndex !== -1) {
                depJob.dependencies.splice(depIndex, 1);
                if (depJob.dependencies.length === 0) {
                  this.addJobToQueues(depJob, queues);
                }
              }
            });
            for (let j = 0; j < workerPool.length; j++) {
              if (workerPool[j].isIdle) {
                this.assignTaskToIdleWorker(workerPool[j], queues, processingJobs, serializableConfig, globalContextPtr);
              }
            }
          }
          if (this.checkAllTasksDone(queues, workerPool)) {
            workers.forEach(worker => worker.postMessage({ type: 'EXIT' }));
            this.processAfterCompile(config, globalContextPtr);
            resolve();
          }
        });
      }
    });
  }

  private updateDependantJobs(jobId: string, processingJobs: Set<string>, jobs: Record<string, Job>, queues: Queues): void {
    finishedJob.push(jobId);
    processingJobs.delete(jobId);
    const completedJob = jobs[jobId];
    completedJob.dependants.forEach(depJobId => {
      const depJob = jobs[depJobId];
      // During incremental compilation, the dependants task does not necessarily exist
      if (!depJob) {
        return;
      }
      const depIndex = depJob.dependencies.indexOf(jobId);
      if (depIndex !== -1) {
        depJob.dependencies.splice(depIndex, 1);
        if (depJob.dependencies.length === 0) {
          this.addJobToQueues(depJob, queues);
        }
      }
    });
  }

  private assignTaskToIdleWorker(
    workerInfo: WorkerInfo,
    queues: Queues,
    processingJobs: Set<string>,
    serializableConfig: Object,
    globalContextPtr: KPointer): void {
    let job: Job | undefined;
    let jobInfo: JobInfo | undefined;

    if (queues.externalProgramQueue.length > 0) {
      job = queues.externalProgramQueue.shift()!;
      jobInfo = {
        id: job.id,
        isCompileAbc: false,
        compileFileInfo: this.allFiles.get(job.fileList[0])!,
        buildConfig: serializableConfig,
        globalContextPtr: globalContextPtr
      };
    }
    else if (queues.abcQueue.length > 0) {
      job = queues.abcQueue.shift()!;
      jobInfo = {
        id: job.id,
        isCompileAbc: true,
        compileFileInfo: this.allFiles.get(job.fileList[0])!,
        buildConfig: serializableConfig,
        globalContextPtr: globalContextPtr
      };
    }

    if (job) {
      processingJobs.add(job.id);
      workerInfo.worker.postMessage({ type: 'ASSIGN_TASK', jobInfo });
      workerInfo.isIdle = false;
    }
  }

  public async runConcurrent(): Promise<void> {
    this.generateModuleInfos();
    if (this.compileFiles.size === 0) {
      return;
    }
    this.generateArkTSConfigForModules();

    const jobs: Record<string, Job> = {};
    const queues: Queues = {
      externalProgramQueue: [],
      abcQueue: [],
    };
    this.initCompileQueues(jobs, queues);

    const processingJobs = new Set<string>();
    const workers: ThreadWorker[] = [];
    await this.invokeWorkers(jobs, queues, processingJobs, workers);
  }
}

interface WorkerInfo {
  worker: ThreadWorker;
  isIdle: boolean;
}

interface Job {
  id: string;
  isDeclFile: boolean;
  isInCycle?: boolean;
  fileList: string[];
  dependencies: string[];
  dependants: string[];
  isAbcJob: boolean;
}

interface Queues {
  externalProgramQueue: Job[];
  abcQueue: Job[];
}

function createHash(str: string): string {
  const hash = crypto.createHash('sha256');
  hash.update(str);
  return hash.digest('hex');
}

// -- runConcurrent code ends --

let finishedJob: string[] = [];
