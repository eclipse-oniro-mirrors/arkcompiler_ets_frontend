import { ArkTSConfigGenerator } from './generate_arktsconfig';
import { ABC_SUFFIX, ETS_SUFFIX } from '../pre_define';
import { BuildConfig, DependencyModuleConfig, ModuleInfo, OHOS_MODULE_TYPE } from '../types';
import { CompileUnitRequest, Ets2panda, NativeCompilerConfigRequest } from '../util/ets2panda';
import { Graph, GraphNode } from '../util/graph';
import { changeFileExtension, resolvePath } from '../util/utils';

export class BaseMode {
  private buildConfig: BuildConfig;
  private compiler: Ets2panda = new Ets2panda();

  public constructor(buildConfig: BuildConfig) {
    this.buildConfig = buildConfig;
  }

  public run(): void {
    let modules: ModuleInfo[] = this.collectModules();
    let graph: Graph<ModuleInfo> = this.createModuleGraph(modules);
    let order: string[] = Graph.topologicalSort(graph);
    let generator: ArkTSConfigGenerator = new ArkTSConfigGenerator(this.buildConfig);
    for (let i: int = 0; i < order.length; i++) {
      let moduleInfo: ModuleInfo = graph.getNodeById(order[i]).data;
      this.compileModule(moduleInfo, generator);
    }
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
      abcPath: this.outputPathForModule(dependency.modulePath, dependency.entryFile, dependency.packageName),
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
      dependencies: this.packageNamesFromDependencies(),
      staticDependencyModules: new Map<string, ModuleInfo>(),
      dynamicDependencyModules: new Map<string, ModuleInfo>(),
      language: undefined,
      abcPath: this.outputPathForModule(moduleRootPath, entryFile, this.buildConfig.packageName),
      frameworkMode: this.buildConfig.frameworkMode,
      useEmptyPackage: this.buildConfig.useEmptyPackage,
      byteCodeHar: this.buildConfig.byteCodeHar,
      staticFiles: staticFiles,
      bundleType: this.buildConfig.bundleType,
      bundleName: this.buildConfig.bundleName,
    };
  }

  private createModuleGraph(modules: ModuleInfo[]): Graph<ModuleInfo> {
    let byPackageName: Map<string, ModuleInfo> = new Map<string, ModuleInfo>();
    for (let i: int = 0; i < modules.length; i++) {
      byPackageName.set(modules[i].packageName, modules[i]);
    }
    let nodes: GraphNode<ModuleInfo>[] = [];
    for (let i: int = 0; i < modules.length; i++) {
      let node: GraphNode<ModuleInfo> = new GraphNode<ModuleInfo>(modules[i].packageName, modules[i]);
      for (let j: int = 0; j < modules[i].dependencies.length; j++) {
        if (byPackageName.has(modules[i].dependencies[j])) {
          node.predecessors.add(modules[i].dependencies[j]);
          modules[i].staticDependencyModules.set(modules[i].dependencies[j], byPackageName.get(modules[i].dependencies[j]) as ModuleInfo);
        }
      }
      nodes.push(node);
    }
    return Graph.createGraphFromNodes(nodes);
  }

  private compileModule(moduleInfo: ModuleInfo, generator: ArkTSConfigGenerator): void {
    generator.generateConfigForModule(moduleInfo);
    let configRequest: NativeCompilerConfigRequest = {
      argv: ['_', '--extension=ets', '--ets-module', '--output=' + this.outputPath(moduleInfo)],
      configText: generator.serializeConfig(moduleInfo.packageName),
    };
    let request: CompileUnitRequest = {
      configRequest: configRequest,
      sourceFile: this.primarySourceFile(moduleInfo),
      sourceFiles: moduleInfo.staticFiles,
      outputPath: this.outputPath(moduleInfo),
    };
    this.compiler.compileExternalSourceSet(request);
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

  private primarySourceFile(moduleInfo: ModuleInfo): string {
    if (moduleInfo.staticFiles.length > 0) {
      return moduleInfo.staticFiles[0];
    }
    return resolvePath(moduleInfo.moduleRootPath, moduleInfo.entryFile);
  }

  private outputPath(moduleInfo: ModuleInfo): string {
    if (moduleInfo.abcPath !== undefined && (moduleInfo.abcPath as string).length > 0) {
      return moduleInfo.abcPath as string;
    }
    return this.outputPathForModule(moduleInfo.moduleRootPath, moduleInfo.entryFile, moduleInfo.packageName);
  }

  private outputPathForModule(moduleRootPath: string, entryFile: string, packageName: string): string {
    if (entryFile.length > 0) {
      return changeFileExtension(resolvePath(moduleRootPath, entryFile), ABC_SUFFIX, ETS_SUFFIX);
    }
    return resolvePath(moduleRootPath, packageName + ABC_SUFFIX);
  }

  private packageNamesFromDependencies(): string[] {
    let dependencies: DependencyModuleConfig[] = this.buildConfig.dependencyModuleList === undefined ? [] : this.buildConfig.dependencyModuleList as DependencyModuleConfig[];
    let result: string[] = [];
    for (let i: int = 0; i < dependencies.length; i++) {
      result.push(dependencies[i].packageName);
    }
    return result;
  }
}
