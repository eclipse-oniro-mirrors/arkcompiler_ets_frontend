import { Logger, LogDataFactory } from '../logger';
import { DriverError, ErrorCode } from '../util/error';
import { isSubPathOf, resolvePath, toUnixPath } from '../util/utils';
import { AliasConfig, ArkTSConfigObject, BuildConfig, CompilerOptions, DependencyItem, ModuleInfo, OHOS_MODULE_TYPE } from '../types';
import { LANGUAGE_VERSION, SYSTEM_SDK_PATH_FROM_SDK } from '../pre_define';

export class ArkTSConfig {
  public configObject: ArkTSConfigObject;

  public constructor(moduleInfo: ModuleInfo, cacheDir: string, projectRootPath: string, declgenV2OutPath: string) {
    this.configObject = {
      compilerOptions: {
        packageName: moduleInfo.packageName,
        baseUrl: resolvePath(moduleInfo.moduleRootPath),
        rootDir: projectRootPath,
        paths: new Map<string, string[]>(),
        dependencies: new Map<string, DependencyItem>(),
        cacheDir: resolvePath(cacheDir),
        declgenV2OutPath: declgenV2OutPath,
      },
      files: moduleInfo.staticFiles ? moduleInfo.staticFiles.slice() : [],
    };
  }

  public get compilerOptions(): CompilerOptions {
    return this.configObject.compilerOptions;
  }

  public get packageName(): string {
    return this.configObject.compilerOptions.packageName;
  }

  public get dependencies(): Map<string, DependencyItem> {
    return this.configObject.compilerOptions.dependencies;
  }

  public get pathSection(): Map<string, string[]> {
    return this.configObject.compilerOptions.paths;
  }

  public set useEmptyPackage(value: boolean) {
    this.configObject.compilerOptions.useEmptyPackage = value;
  }

  public addPathMappings(mappings: Map<string, string[]>): void {
    mappings.forEach((value: string[], key: string) => {
      let existing: string[] | undefined = this.pathSection.get(key);
      if (existing === undefined) {
        this.pathSection.set(key, this.deduplicateStrings(value));
        return;
      }
      this.pathSection.set(key, this.deduplicateStrings((existing as string[]).concat(value)));
    });
  }

  public addPathMapping(name: string, paths: string[]): void {
    let mappings: Map<string, string[]> = new Map<string, string[]>();
    mappings.set(name, paths);
    this.addPathMappings(mappings);
  }

  public addDependency(name: string, item: DependencyItem): void {
    let existing: DependencyItem | undefined = this.dependencies.get(name);
    if (existing === undefined) {
      this.dependencies.set(name, this.cloneDependencyItem(item));
      return;
    }
    let merged: DependencyItem = this.cloneDependencyItem(existing as DependencyItem);
    merged.language = item.language;
    merged.path = item.path;
    merged.ohmUrl = item.ohmUrl;
    if (item.sourceFilePath !== undefined) {
      merged.sourceFilePath = item.sourceFilePath;
    }
    if (item.mainFile !== undefined) {
      merged.mainFile = item.mainFile;
    }
    merged.alias = this.deduplicateStrings(this.getAliasArray(existing as DependencyItem).concat(this.getAliasArray(item)));
    this.dependencies.set(name, merged);
  }

  public addDependencies(deps: Map<string, DependencyItem>): void {
    deps.forEach((item: DependencyItem, name: string) => {
      this.addDependency(name, item);
    });
  }

  public mergeArktsConfig(source: ArkTSConfig | undefined): void {
    if (source === undefined) {
      return;
    }
    this.addDependencies(source.dependencies);
    this.addPathMappings(source.pathSection);
  }

  public mergeArktsConfigByDependencies(dependencies: Set<string>, dependenciesSets: Map<string, Set<string>>, generator: ArkTSConfigGenerator): void {
    let processed: Set<string> = new Set<string>();
    dependencies.forEach((dependency: string) => {
      this.mergeDependencyRecursive(dependency, dependenciesSets, processed, generator);
    });
  }

  public toJSONText(): string {
    let filesText: string = this.configObject.files === undefined ? '[]' : JSON.stringify(this.configObject.files as string[]);
    return '{"compilerOptions":' + this.compilerOptionsToJSONText() + ',"files":' + filesText + '}';
  }

  private mergeDependencyRecursive(dependency: string, dependenciesSets: Map<string, Set<string>>, processed: Set<string>, generator: ArkTSConfigGenerator): void {
    if (processed.has(dependency)) {
      return;
    }
    processed.add(dependency);
    let dependencyDependencies: Set<string> | undefined = dependenciesSets.get(dependency);
    if (dependencyDependencies !== undefined) {
      (dependencyDependencies as Set<string>).forEach((dep: string) => {
        this.mergeDependencyRecursive(dep, dependenciesSets, processed, generator);
      });
    }
    this.mergeArktsConfig(generator.getArktsConfigByPackageName(dependency));
  }

  private cloneDependencyItem(item: DependencyItem): DependencyItem {
    let clone: DependencyItem = {
      language: item.language,
      path: item.path,
      ohmUrl: item.ohmUrl,
    };
    if (item.sourceFilePath !== undefined) {
      clone.sourceFilePath = item.sourceFilePath;
    }
    if (item.alias !== undefined) {
      clone.alias = this.deduplicateStrings(item.alias as string[]);
    }
    if (item.mainFile !== undefined) {
      clone.mainFile = item.mainFile;
    }
    return clone;
  }

  private getAliasArray(item: DependencyItem): string[] {
    if (item.alias === undefined) {
      return [];
    }
    return (item.alias as string[]).slice();
  }

  private deduplicateStrings(items: string[]): string[] {
    let result: string[] = [];
    let seen: Set<string> = new Set<string>();
    for (let i: int = 0; i < items.length; i++) {
      if (!seen.has(items[i])) {
        seen.add(items[i]);
        result.push(items[i]);
      }
    }
    return result;
  }

  private compilerOptionsToJSONText(): string {
    let pairs: string[] = [];
    pairs.push('"package":' + JSON.stringify(this.packageName));
    pairs.push('"baseUrl":' + JSON.stringify(this.compilerOptions.baseUrl));
    pairs.push('"rootDir":' + JSON.stringify(this.compilerOptions.rootDir));
    pairs.push('"paths":' + this.pathMapToJSONText(this.pathSection));
    pairs.push('"dependencies":' + this.dependenciesToJSONText(this.dependencies));
    if (this.compilerOptions.cacheDir !== undefined) {
      pairs.push('"cacheDir":' + JSON.stringify(this.compilerOptions.cacheDir as string));
    }
    if (this.compilerOptions.declgenV2OutPath !== undefined) {
      pairs.push('"declgenV2OutPath":' + JSON.stringify(this.compilerOptions.declgenV2OutPath as string));
    }
    if (this.compilerOptions.useEmptyPackage !== undefined) {
      pairs.push('"useEmptyPackage":' + JSON.stringify(this.compilerOptions.useEmptyPackage as boolean));
    }
    return '{' + pairs.join(',') + '}';
  }

  private pathMapToJSONText(map: Map<string, string[]>): string {
    let pairs: string[] = [];
    map.forEach((value: string[], key: string) => {
      pairs.push(JSON.stringify(key) + ':' + JSON.stringify(value));
    });
    return '{' + pairs.join(',') + '}';
  }

  private dependenciesToJSONText(map: Map<string, DependencyItem>): string {
    let pairs: string[] = [];
    map.forEach((value: DependencyItem, key: string) => {
      pairs.push(JSON.stringify(key) + ':' + JSON.stringify(this.cloneDependencyItem(value)));
    });
    return '{' + pairs.join(',') + '}';
  }
}

export class ArkTSConfigGenerator {
  private systemSdkPath: string;
  private stdlibAbcPath: string;
  private buildConfig: BuildConfig;
  private logger: Logger;
  private systemPathSection: Map<string, string[]> = new Map<string, string[]>();
  private systemDependenciesSection: Map<string, DependencyItem> = new Map<string, DependencyItem>();
  private arktsconfigs: Map<string, ArkTSConfig> = new Map<string, ArkTSConfig>();

  public constructor(buildConfig: BuildConfig) {
    this.logger = Logger.getInstance();
    this.buildConfig = buildConfig;
    this.systemSdkPath = resolvePath(buildConfig.buildSdkPath === undefined ? '' : buildConfig.buildSdkPath as string, SYSTEM_SDK_PATH_FROM_SDK);
    let pandaSdkPath: string = buildConfig.pandaSdkPath === undefined ? '' : buildConfig.pandaSdkPath as string;
    let pandaStdlibPath: string = buildConfig.pandaStdlibPath === undefined ? resolvePath(pandaSdkPath, 'lib', 'etsstdlib.abc') : buildConfig.pandaStdlibPath as string;
    this.stdlibAbcPath = resolvePath(pandaStdlibPath);
    this.initPathInfo();
  }

  public get aliasConfig(): Map<string, Map<string, AliasConfig>> {
    if (this.buildConfig.aliasConfig === undefined) {
      return new Map<string, Map<string, AliasConfig>>();
    }
    return this.buildConfig.aliasConfig as Map<string, Map<string, AliasConfig>>;
  }

  public get dynamicSDKPaths(): Set<string> {
    if (this.buildConfig.interopSDKPaths === undefined) {
      return new Set<string>();
    }
    return this.buildConfig.interopSDKPaths as Set<string>;
  }

  public get externalApiPaths(): string[] {
    if (this.buildConfig.externalApiPaths === undefined) {
      return [];
    }
    return this.buildConfig.externalApiPaths as string[];
  }

  public generateArkTSConfigFile(moduleInfo: ModuleInfo, enableDeclgenEts2Ts: boolean): ArkTSConfig {
    if (moduleInfo.sourceRoots.length === 0) {
      throw new DriverError(LogDataFactory.newInstance(
        ErrorCode.BUILDSYSTEM_SOURCEROOTS_NOT_SET_FAIL,
        'SourceRoots not set for module ' + moduleInfo.packageName + '.'
      ));
    }
    let cachePath: string = this.buildConfig.cachePath === undefined ? '' : this.buildConfig.cachePath as string;
    let projectRootPath: string = this.buildConfig.projectRootPath === undefined ? moduleInfo.moduleRootPath : this.buildConfig.projectRootPath as string;
    let declgenV2OutPath: string = this.buildConfig.declgenV2OutPath === undefined ? '' : this.buildConfig.declgenV2OutPath as string;
    let arktsConfig: ArkTSConfig = new ArkTSConfig(moduleInfo, cachePath, projectRootPath, declgenV2OutPath);
    this.arktsconfigs.set(moduleInfo.packageName, arktsConfig);
    this.addPathSection(moduleInfo, arktsConfig);
    if (!enableDeclgenEts2Ts) {
      this.addDependenciesSection(moduleInfo, arktsConfig);
    }
    this.addEtsStdLibToDependencySection(arktsConfig);
    this.addBinaryDependencySection(moduleInfo, arktsConfig);
    this.processAlias(arktsConfig);
    if (this.buildConfig.frameworkMode === true) {
      arktsConfig.useEmptyPackage = this.buildConfig.useEmptyPackage === true;
    }
    this.logger.printDebug('arktsconfig for ' + moduleInfo.packageName + ':\n' + arktsConfig.toJSONText());
    return arktsConfig;
  }

  public generateConfigForModule(moduleInfo: ModuleInfo): ArkTSConfig {
    return this.generateArkTSConfigFile(moduleInfo, this.buildConfig.enableDeclgenEts2Ts === true);
  }

  public serializeConfig(packageName: string): string {
    let config: ArkTSConfig | undefined = this.getArktsConfigByPackageName(packageName);
    if (config === undefined) {
      return '';
    }
    return (config as ArkTSConfig).toJSONText();
  }

  public getArktsConfigByPackageName(packageName: string): ArkTSConfig | undefined {
    return this.arktsconfigs.get(packageName);
  }

  public compareConfigSnapshot(packageName: string, compilerOptions: string, files: string, paths: string): boolean {
    let config: ArkTSConfig | undefined = this.getArktsConfigByPackageName(packageName);
    if (config === undefined) {
      return false;
    }
    let actual: ArkTSConfig = config as ArkTSConfig;
    return actual.toJSONText().indexOf(compilerOptions) >= 0 &&
      actual.toJSONText().indexOf(files) >= 0 &&
      actual.toJSONText().indexOf(paths) >= 0;
  }

  private initPathInfo(): void {
    if (this.buildConfig.paths !== undefined) {
      (this.buildConfig.paths as Map<string, string[]>).forEach((value: string[], key: string) => {
        this.systemPathSection.set(key, value.slice());
      });
    }
    this.addKnownSdkPathMappings();
  }

  private addKnownSdkPathMappings(): void {
    let sdkRoots: string[] = this.externalApiPaths;
    if (sdkRoots.length === 0 && this.systemSdkPath.length > 0) {
      sdkRoots = [resolvePath(this.systemSdkPath, 'api'), resolvePath(this.systemSdkPath, 'arkts'), resolvePath(this.systemSdkPath, 'kits')];
    }
    for (let i: int = 0; i < sdkRoots.length; i++) {
      let root: string = resolvePath(sdkRoots[i]);
      if (root.length > 0) {
        this.systemPathSection.set(root, [root]);
      }
    }
  }

  private addPathSection(moduleInfo: ModuleInfo, arktsConfig: ArkTSConfig): void {
    arktsConfig.addPathMappings(this.systemPathSection);
    if (moduleInfo.language === LANGUAGE_VERSION.ARKTS_1_1) {
      return;
    }
    this.getAllFilesToPathSection(moduleInfo, arktsConfig);
    this.getAllModuleFilesToPathSection(moduleInfo, arktsConfig);
    if (moduleInfo.packageName.length > 0) {
      this.handleAddSourceRootPath(moduleInfo, arktsConfig);
    }
  }

  private handleAddSourceRootPath(moduleInfo: ModuleInfo, arktsConfig: ArkTSConfig): void {
    let paths: string[] = [];
    for (let i: int = moduleInfo.sourceRoots.length - 1; i >= 0; i--) {
      paths.push(resolvePath(moduleInfo.moduleRootPath, moduleInfo.sourceRoots[i]));
    }
    paths.push(resolvePath(moduleInfo.moduleRootPath, './'));
    arktsConfig.addPathMapping(moduleInfo.packageName, paths);
    if (moduleInfo.entryFile.length > 0) {
      arktsConfig.addPathMapping(moduleInfo.packageName + '/Index', [resolvePath(moduleInfo.moduleRootPath, moduleInfo.entryFile)]);
    }
  }

  private getAllFilesToPathSection(moduleInfo: ModuleInfo, arktsConfig: ArkTSConfig): void {
    if (this.buildConfig.compileFiles === undefined) {
      return;
    }
    let files: string[] = this.buildConfig.compileFiles as string[];
    for (let i: int = 0; i < files.length; i++) {
      this.addFilePathMapping(files[i], moduleInfo, arktsConfig);
    }
  }

  private getAllModuleFilesToPathSection(moduleInfo: ModuleInfo, arktsConfig: ArkTSConfig): void {
    for (let i: int = 0; i < moduleInfo.staticFiles.length; i++) {
      this.addFilePathMapping(moduleInfo.staticFiles[i], moduleInfo, arktsConfig);
    }
  }

  private addFilePathMapping(file: string, moduleInfo: ModuleInfo, arktsConfig: ArkTSConfig): void {
    let moduleRoot: string = toUnixPath(moduleInfo.moduleRootPath);
    moduleRoot += moduleRoot.endsWith('/') ? '' : '/';
    let unixFilePath: string = toUnixPath(file);
    if (!isSubPathOf(unixFilePath, moduleRoot)) {
      return;
    }
    let relativePath: string = unixFilePath.substring(moduleRoot.length);
    let keyWithoutExtension: string = this.removeSourceExtension(relativePath);
    arktsConfig.addPathMapping(moduleInfo.packageName + '/' + keyWithoutExtension, [file]);
  }

  private removeSourceExtension(file: string): string {
    if (file.endsWith('.d.ets')) {
      return file.substring(0, file.length - 6);
    }
    let index: int = file.lastIndexOf('.');
    if (index < 0) {
      return file;
    }
    return file.substring(0, index);
  }

  private addDependenciesSection(moduleInfo: ModuleInfo, arktsConfig: ArkTSConfig): void {
    moduleInfo.dynamicDependencyModules.forEach((depModuleInfo: ModuleInfo) => {
      this.addSingleModuleDependencies(moduleInfo, depModuleInfo, arktsConfig);
    });
    arktsConfig.addDependencies(this.systemDependenciesSection);
  }

  private addSingleModuleDependencies(moduleInfo: ModuleInfo, depModuleInfo: ModuleInfo, arktsConfig: ArkTSConfig): void {
    let alias: string = this.getAliasForPackage(depModuleInfo.packageName, moduleInfo.originalPackageNameMap);
    for (let i: int = 0; i < depModuleInfo.staticFiles.length; i++) {
      let file: string = depModuleInfo.staticFiles[i];
      let key: string = this.getDependencyKey(file, depModuleInfo);
      let depItem: DependencyItem = {
        language: depModuleInfo.language === LANGUAGE_VERSION.ARKTS_1_1 ? 'js' : 'ets',
        path: depModuleInfo.declFilesPath === undefined ? file : depModuleInfo.declFilesPath as string,
        sourceFilePath: file,
        ohmUrl: depModuleInfo.packageName,
      };
      arktsConfig.addDependency(key, depItem);
      if (this.isPackageEntryFile(file, depModuleInfo)) {
        arktsConfig.addDependency(depModuleInfo.packageName, depItem);
      }
      if (alias.length > 0) {
        let aliasKey: string = alias + key.substring(depModuleInfo.packageName.length);
        arktsConfig.addDependency(aliasKey, depItem);
        if (this.isPackageEntryFile(file, depModuleInfo)) {
          arktsConfig.addDependency(alias, depItem);
        }
      }
    }
  }

  private getDependencyKey(file: string, moduleInfo: ModuleInfo): string {
    let normalizedFile: string = toUnixPath(file);
    let moduleRoot: string = toUnixPath(moduleInfo.moduleRootPath);
    moduleRoot += moduleRoot.endsWith('/') ? '' : '/';
    if (isSubPathOf(normalizedFile, moduleRoot)) {
      return moduleInfo.packageName + '/' + this.removeSourceExtension(normalizedFile.substring(moduleRoot.length));
    }
    return moduleInfo.packageName + '/' + this.removeSourceExtension(normalizedFile);
  }

  private isPackageEntryFile(file: string, depModuleInfo: ModuleInfo): boolean {
    return this.removeSourceExtension(toUnixPath(file)).endsWith(this.removeSourceExtension(toUnixPath(depModuleInfo.entryFile)));
  }

  private addEtsStdLibToDependencySection(arktsconfig: ArkTSConfig): void {
    let names: string[] = [
      'std/core',
      'std/math',
      'std/math/consts',
      'std/containers',
      'std/interop/js',
      'std/time',
      'std/debug',
      'std/debug/concurrency',
      'std/dfx',
      'std/testing',
      'std/concurrency',
      'std/annotations',
      'std/interop',
      'escompat',
      'arkruntime',
    ];
    for (let i: int = 0; i < names.length; i++) {
      arktsconfig.addDependency(names[i], { language: 'ets', path: this.stdlibAbcPath, ohmUrl: names[i] });
    }
  }

  private addBinaryDependencySection(moduleInfo: ModuleInfo, arktsconfig: ArkTSConfig): void {
    moduleInfo.staticDependencyModules.forEach((depModuleInfo: ModuleInfo) => {
      if (arktsconfig.dependencies.has(depModuleInfo.packageName)) {
        return;
      }
      if (depModuleInfo.moduleType === OHOS_MODULE_TYPE.HAR && depModuleInfo.abcPath !== undefined) {
        let depItem: DependencyItem = {
          language: 'ets',
          path: depModuleInfo.abcPath as string,
          ohmUrl: depModuleInfo.packageName,
          mainFile: 'Index',
        };
        arktsconfig.addDependency(depModuleInfo.packageName, depItem);
        let alias: string = this.getAliasForPackage(depModuleInfo.packageName, moduleInfo.originalPackageNameMap);
        if (alias.length > 0) {
          arktsconfig.addDependency(alias, depItem);
        }
      }
    });
  }

  private processAlias(arktsConfig: ArkTSConfig): void {
    let aliasForPkg: Map<string, AliasConfig> | undefined = this.aliasConfig.get(arktsConfig.packageName);
    if (aliasForPkg === undefined) {
      return;
    }
    (aliasForPkg as Map<string, AliasConfig>).forEach((aliasConfig: AliasConfig, aliasName: string) => {
      if (aliasConfig.isStatic) {
        return;
      }
      arktsConfig.addDependency(aliasName, {
        language: 'js',
        path: aliasConfig.originalAPIName,
        ohmUrl: aliasConfig.originalAPIName,
        alias: [aliasName],
      });
    });
  }

  private getAliasForPackage(packageName: string, originalPackageNameMap?: Map<string, string>): string {
    if (originalPackageNameMap === undefined) {
      return '';
    }
    let result: string = '';
    (originalPackageNameMap as Map<string, string>).forEach((originalName: string, alias: string) => {
      if (originalName === packageName && alias !== originalName) {
        result = alias;
      }
    });
    return result;
  }
}

export function generateArkTSConfig(): void {
}
