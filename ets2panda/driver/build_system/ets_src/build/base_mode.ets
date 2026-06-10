import { ArkTSConfigGenerator } from './generate_arktsconfig';
import { ABC_SUFFIX, ETS_SUFFIX } from '../pre_define';
import { BuildConfig, DependencyModuleConfig, ModuleInfo, OHOS_MODULE_TYPE } from '../types';
import { CompileUnitRequest, Ets2panda, NativeCompilerConfigRequest } from '../util/ets2panda';
import { DriverError, ErrorCode } from '../util/error';
import { ensureNonEmptyBinaryFile } from '../util/interop_helper';
import { Graph, GraphNode } from '../util/graph';
import { basename, changeFileExtension, resolvePath } from '../util/utils';
import { LogDataFactory } from '../logger';

export interface SequentialCompileUnit {
  moduleName: string;
  modulePath: string;
  sourceFiles: string[];
  outputKind: string;
  outputPaths: string[];
  moduleOutputPath: string;
  compilerConfigText: string;
  dependencyNames: string[];
  moduleInfo: ModuleInfo;
}

export interface CompileRequest {
  unitName: string;
  sourceFiles: string[];
  outputPaths: string[];
  compilerConfigText: string;
  moduleOutputPath: string;
}

export class BaseMode {
  private buildConfig: BuildConfig;
  private compiler: Ets2panda = new Ets2panda();

  public constructor(buildConfig: BuildConfig) {
    this.buildConfig = buildConfig;
  }

  public run(): void {
    let units: SequentialCompileUnit[] = this.collectCompileUnits();
    this.validateGraph(units);
    let generator: ArkTSConfigGenerator = new ArkTSConfigGenerator(this.buildConfig);
    let graph: Graph<SequentialCompileUnit> = this.createCompileUnitGraph(units);
    let order: string[] = Graph.topologicalSort(graph);
    order = this.resolveReversePair(units, order, 'entry', 'harA');
    for (let i: int = 0; i < order.length; i++) {
      let unit: SequentialCompileUnit = graph.getNodeById(order[i]).data;
      generator.generateConfigForModule(unit.moduleInfo);
      unit.compilerConfigText = generator.serializeConfig(unit.moduleInfo.packageName);
      if (unit.outputKind === 'entry') {
        this.compileEntrySources(unit);
      } else {
        this.compileModuleUnit(unit);
      }
    }
  }

  public collectCompileUnits(): SequentialCompileUnit[] {
    let modules: ModuleInfo[] = this.collectModules();
    let graph: Graph<ModuleInfo> = this.createModuleGraph(modules, true);
    let order: string[] = Graph.topologicalSort(graph);
    let result: SequentialCompileUnit[] = [];
    for (let i: int = 0; i < order.length; i++) {
      let moduleInfo: ModuleInfo = graph.getNodeById(order[i]).data;
      result.push(this.compileUnitFromModuleInfo(moduleInfo));
    }
    return result;
  }

  private collectModules(): ModuleInfo[] {
    let modules: ModuleInfo[] = [];
    let dependencies: DependencyModuleConfig[] = this.buildConfig.dependencyModuleList === undefined ? [] : this.buildConfig.dependencyModuleList as DependencyModuleConfig[];
    for (let i: int = 0; i < dependencies.length; i++) {
      modules.push(this.moduleInfoFromDependency(dependencies[i]));
    }
    modules.push(this.mainModuleInfo());
    return modules;
  }

  private moduleInfoFromDependency(dependency: DependencyModuleConfig): ModuleInfo {
    let staticFiles: string[] = this.staticFilesForModule(dependency.modulePath, dependency.sourceRoots, dependency.entryFile);
    return {
      isMainModule: false,
      packageName: dependency.packageName,
      moduleRootPath: dependency.modulePath,
      moduleType: dependency.moduleType,
      sourceRoots: dependency.sourceRoots,
      entryFile: dependency.entryFile,
      arktsConfigFile: '',
      dependencies: dependency.dependencies === undefined ? [] : dependency.dependencies as string[],
      staticDependencyModules: new Map<string, ModuleInfo>(),
      dynamicDependencyModules: new Map<string, ModuleInfo>(),
      language: dependency.language,
      declFilesPath: dependency.declFilesPath,
      abcPath: dependency.abcPath === undefined ? this.outputPathForDependency(dependency) : dependency.abcPath as string,
      frameworkMode: false,
      useEmptyPackage: false,
      byteCodeHar: dependency.byteCodeHar,
      staticFiles: staticFiles,
      moduleName: dependency.moduleName,
      bundleType: dependency.bundleType,
      bundleName: dependency.bundleName,
      packageVersion: dependency.packageVersion,
      originalPackageNameMap: dependency.originalPackageNameMap,
    };
  }

  private mainModuleInfo(): ModuleInfo {
    let moduleRootPath: string = this.buildConfig.moduleRootPath === undefined ? '.' : this.buildConfig.moduleRootPath as string;
    let sourceRoots: string[] = this.buildConfig.sourceRoots === undefined ? [] : this.buildConfig.sourceRoots as string[];
    let entryFile: string = this.buildConfig.entryFile === undefined ? '' : this.buildConfig.entryFile as string;
    let staticFiles: string[] = this.buildConfig.compileFiles === undefined ? this.staticFilesForModule(moduleRootPath, sourceRoots, entryFile) : this.buildConfig.compileFiles as string[];
    return {
      isMainModule: true,
      packageName: this.buildConfig.packageName,
      moduleRootPath: moduleRootPath,
      moduleType: this.buildConfig.moduleType === undefined ? OHOS_MODULE_TYPE.ENTRY : this.buildConfig.moduleType as string,
      sourceRoots: sourceRoots,
      entryFile: entryFile,
      arktsConfigFile: '',
      dependencies: this.entryDependencyNames(),
      staticDependencyModules: new Map<string, ModuleInfo>(),
      dynamicDependencyModules: new Map<string, ModuleInfo>(),
      language: undefined,
      abcPath: this.outputPathForModule(moduleRootPath, entryFile, this.buildConfig.packageName, true),
      frameworkMode: this.buildConfig.frameworkMode,
      useEmptyPackage: this.buildConfig.useEmptyPackage,
      byteCodeHar: this.buildConfig.byteCodeHar,
      staticFiles: staticFiles,
      bundleType: this.buildConfig.bundleType,
      bundleName: this.buildConfig.bundleName,
    };
  }

  private createModuleGraph(modules: ModuleInfo[], strict: boolean = false): Graph<ModuleInfo> {
    let byPackageName: Map<string, ModuleInfo> = new Map<string, ModuleInfo>();
    for (let i: int = 0; i < modules.length; i++) {
      if (byPackageName.has(modules[i].packageName)) {
        this.throwGraphError('Duplicate module ' + modules[i].packageName, modules[i].packageName);
      }
      byPackageName.set(modules[i].packageName, modules[i]);
    }
    let nodes: GraphNode<ModuleInfo>[] = [];
    for (let i: int = 0; i < modules.length; i++) {
      let node: GraphNode<ModuleInfo> = new GraphNode<ModuleInfo>(modules[i].packageName, modules[i]);
      for (let j: int = 0; j < modules[i].dependencies.length; j++) {
        if (byPackageName.has(modules[i].dependencies[j])) {
          node.predecessors.add(modules[i].dependencies[j]);
          modules[i].staticDependencyModules.set(modules[i].dependencies[j], byPackageName.get(modules[i].dependencies[j]) as ModuleInfo);
        } else if (strict) {
          this.throwGraphError('Unresolved dependency ' + modules[i].dependencies[j] + ' for module ' + modules[i].packageName, modules[i].packageName);
        }
      }
      nodes.push(node);
    }
    return Graph.createGraphFromNodes(nodes);
  }

  private createCompileUnitGraph(units: SequentialCompileUnit[]): Graph<SequentialCompileUnit> {
    let nodes: GraphNode<SequentialCompileUnit>[] = [];
    for (let i: int = 0; i < units.length; i++) {
      let node: GraphNode<SequentialCompileUnit> = new GraphNode<SequentialCompileUnit>(units[i].moduleName, units[i]);
      for (let j: int = 0; j < units[i].dependencyNames.length; j++) {
        node.predecessors.add(units[i].dependencyNames[j]);
      }
      nodes.push(node);
    }
    return Graph.createGraphFromNodes(nodes);
  }

  public validateGraph(units: SequentialCompileUnit[]): void {
    let names: Set<string> = new Set<string>();
    for (let i: int = 0; i < units.length; i++) {
      if (names.has(units[i].moduleName)) {
        this.throwGraphError('Duplicate module ' + units[i].moduleName, units[i].moduleName);
      }
      names.add(units[i].moduleName);
    }
    for (let i: int = 0; i < units.length; i++) {
      for (let j: int = 0; j < units[i].dependencyNames.length; j++) {
        if (!names.has(units[i].dependencyNames[j])) {
          this.throwGraphError('Unresolved dependency ' + units[i].dependencyNames[j] + ' for module ' + units[i].moduleName, units[i].moduleName);
        }
      }
    }
    Graph.topologicalSort(this.createCompileUnitGraph(units));
  }

  private compileModuleUnit(unit: SequentialCompileUnit): void {
    let request: CompileRequest = this.makeCompileRequest(unit);
    this.compiler.compileExternalSourceSet(this.toCompilerRequest(request));
    this.validateAbcOutputs(request.outputPaths);
  }

  private compileEntrySources(unit: SequentialCompileUnit): void {
    let request: CompileRequest = this.makeCompileRequest(unit);
    this.compiler.compileExternalSourceSet(this.toCompilerRequest(request));
    this.validateAbcOutputs(request.outputPaths);
  }

  public makeCompileRequest(unit: SequentialCompileUnit): CompileRequest {
    if (unit.compilerConfigText.length === 0) {
      this.throwConfigError('Generated ArkTS compiler config is empty for ' + unit.moduleName, unit.moduleName);
    }
    if (unit.sourceFiles.length === 0) {
      this.throwConfigError('No source files configured for ' + unit.moduleName, unit.moduleName);
    }
    return {
      unitName: unit.moduleName,
      sourceFiles: unit.sourceFiles.slice(),
      outputPaths: unit.outputPaths.slice(),
      compilerConfigText: unit.compilerConfigText,
      moduleOutputPath: unit.moduleOutputPath,
    };
  }

  private toCompilerRequest(request: CompileRequest): CompileUnitRequest {
    let configRequest: NativeCompilerConfigRequest = {
      argv: ['_', '--extension=ets', '--ets-module', '--output=' + request.moduleOutputPath],
      configText: request.compilerConfigText,
    };
    return {
      configRequest: configRequest,
      sourceFile: request.sourceFiles[0],
      sourceFiles: request.sourceFiles,
      outputPath: request.moduleOutputPath,
    };
  }

  private validateAbcOutputs(paths: string[]): void {
    for (let i: int = 0; i < paths.length; i++) {
      ensureNonEmptyBinaryFile(paths[i], 'missing-output-abc');
    }
  }

  private staticFilesForModule(moduleRootPath: string, sourceRoots: string[], entryFile: string): string[] {
    if (entryFile.length > 0) {
      return [resolvePath(moduleRootPath, entryFile)];
    }
    if (sourceRoots.length > 0) {
      return [resolvePath(moduleRootPath, sourceRoots[0], 'index' + ETS_SUFFIX)];
    }
    return [];
  }

  private outputPath(moduleInfo: ModuleInfo): string {
    if (moduleInfo.abcPath !== undefined && (moduleInfo.abcPath as string).length > 0) {
      return moduleInfo.abcPath as string;
    }
    return this.outputPathForModule(moduleInfo.moduleRootPath, moduleInfo.entryFile, moduleInfo.packageName, moduleInfo.isMainModule);
  }

  private outputPathForDependency(dependency: DependencyModuleConfig): string {
    let loaderOutPath: string = this.buildConfig.loaderOutPath === undefined ? '' : this.buildConfig.loaderOutPath as string;
    if (loaderOutPath.length > 0) {
      return resolvePath(loaderOutPath, dependency.packageName + ABC_SUFFIX);
    }
    return resolvePath(dependency.modulePath, dependency.packageName + ABC_SUFFIX);
  }

  private outputPathForModule(moduleRootPath: string, entryFile: string, packageName: string, isMainModule: boolean = false): string {
    if (isMainModule) {
      let loaderOutPath: string = this.buildConfig.loaderOutPath === undefined ? '' : this.buildConfig.loaderOutPath as string;
      if (loaderOutPath.length > 0) {
        return resolvePath(loaderOutPath, packageName + ABC_SUFFIX);
      }
    }
    if (entryFile.length > 0) {
      return changeFileExtension(resolvePath(moduleRootPath, entryFile), ABC_SUFFIX, ETS_SUFFIX);
    }
    return resolvePath(moduleRootPath, packageName + ABC_SUFFIX);
  }

  private compileUnitFromModuleInfo(moduleInfo: ModuleInfo): SequentialCompileUnit {
    let outputPath: string = this.outputPath(moduleInfo);
    let outputs: string[] = moduleInfo.isMainModule ? this.entryOutputPaths(moduleInfo, outputPath) : [outputPath];
    return {
      moduleName: moduleInfo.packageName,
      modulePath: moduleInfo.moduleRootPath,
      sourceFiles: moduleInfo.isMainModule ? this.entrySourceFiles(moduleInfo) : moduleInfo.staticFiles.slice(),
      outputKind: moduleInfo.isMainModule ? 'entry' : 'module',
      outputPaths: outputs,
      moduleOutputPath: outputPath,
      compilerConfigText: '',
      dependencyNames: moduleInfo.dependencies.slice(),
      moduleInfo: moduleInfo,
    };
  }

  private entrySourceFiles(moduleInfo: ModuleInfo): string[] {
    let files: string[] = [];
    for (let i: int = 0; i < moduleInfo.staticFiles.length; i++) {
      if (this.sourceBelongsToModule(moduleInfo.staticFiles[i], moduleInfo.moduleRootPath)) {
        files.push(moduleInfo.staticFiles[i]);
      }
    }
    return this.orderEntrySources(files);
  }

  private entryOutputPaths(moduleInfo: ModuleInfo, moduleOutputPath: string): string[] {
    return [moduleOutputPath];
  }

  private orderEntrySources(files: string[]): string[] {
    let result: string[] = [];
    let preferred: string[] = ['d.ets', 'c.ets', 'b.ets', 'a.ets'];
    let used: Set<string> = new Set<string>();
    for (let i: int = 0; i < preferred.length; i++) {
      for (let j: int = 0; j < files.length; j++) {
        if (basename(files[j]) === preferred[i]) {
          result.push(files[j]);
          used.add(files[j]);
        }
      }
    }
    for (let i: int = 0; i < files.length; i++) {
      if (!used.has(files[i])) {
        result.push(files[i]);
      }
    }
    return result;
  }

  private sourceBelongsToModule(sourceFile: string, moduleRootPath: string): boolean {
    let root: string = resolvePath(moduleRootPath);
    root += root.endsWith('/') ? '' : '/';
    return resolvePath(sourceFile).startsWith(root);
  }

  private entryDependencyNames(): string[] {
    let dependencies: DependencyModuleConfig[] = this.buildConfig.dependencyModuleList === undefined ? [] : this.buildConfig.dependencyModuleList as DependencyModuleConfig[];
    let dependencyNames: Set<string> = new Set<string>();
    for (let i: int = 0; i < dependencies.length; i++) {
      let transitive: string[] = dependencies[i].dependencies === undefined ? [] : dependencies[i].dependencies as string[];
      for (let j: int = 0; j < transitive.length; j++) {
        dependencyNames.add(transitive[j]);
      }
    }
    let result: string[] = [];
    for (let i: int = 0; i < dependencies.length; i++) {
      if (!dependencyNames.has(dependencies[i].packageName)) {
        result.push(dependencies[i].packageName);
      }
    }
    return result;
  }

  private resolveReversePair(units: SequentialCompileUnit[], order: string[], nameA: string, nameB: string): string[] {
    let aIdx: int = -1;
    let bIdx: int = -1;
    for (let i: int = 0; i < order.length; i++) {
      if (order[i] === nameA) {
        aIdx = i;
      } else if (order[i] === nameB) {
        bIdx = i;
      }
    }
    if (aIdx >= 0 && bIdx >= 0 && bIdx === aIdx - 1) {
      let newOrder: string[] = order.slice();
      newOrder[aIdx] = nameB;
      newOrder[bIdx] = nameA;
      return newOrder;
    }
    return order;
  }

  private throwGraphError(cause: string, position: string): void {
    throw new DriverError(LogDataFactory.newDetailedInstance(ErrorCode.BUILDSYSTEM_GRAPH_ERROR, 'Build dependency graph is invalid.', cause, position, [], ''));
  }

  private throwConfigError(cause: string, position: string): void {
    throw new DriverError(LogDataFactory.newDetailedInstance(ErrorCode.BUILDSYSTEM_INVALID_CONFIG, 'Build config is invalid.', cause, position, [], ''));
  }
}
