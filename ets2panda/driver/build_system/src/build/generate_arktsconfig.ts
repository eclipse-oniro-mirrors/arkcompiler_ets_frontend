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

import * as path from 'path';
import * as fs from 'fs';

import {
    Logger,
    LogDataFactory
} from '../logger';
import {
    ErrorCode,
    DriverError
} from '../util/error';
import {
    changeFileExtension,
    ensurePathExists,
    getInteropFilePathByApi,
    getOhmurlByApi,
    isSubPathOf,
    safeRealpath,
    toUnixPath
} from '../util/utils';
import {
    AliasConfig,
    CompilerOptions,
    ArkTSConfigObject,
    BuildConfig,
    DependencyItem,
    DynamicFileContext,
    ModuleInfo,
} from '../types';
import {
    COMPONENT,
    DYNAMIC_PREFIX,
    KITS,
    LANGUAGE_VERSION,
    SYSTEM_SDK_PATH_FROM_SDK,
    sdkConfigPrefix,
} from '../pre_define';

export class ArkTSConfig {
    object: ArkTSConfigObject;

    constructor(moduleInfo: ModuleInfo, cacheDir: string, projectRootPath: string) {
        this.object = {
            compilerOptions: {
                package: moduleInfo.packageName,
                baseUrl: path.resolve(moduleInfo.moduleRootPath, moduleInfo.sourceRoots[0]),
                paths: {},
                dependencies: {},
                cacheDir: path.resolve(cacheDir),
                projectRootPath: projectRootPath,
            }
        };
    }

    addPathMappings(mappings: Record<string, string[]>): void {
        const paths = this.compilerOptions.paths;
        for (const [key, value] of Object.entries(mappings)) {
            if (!paths[key]) {
                paths[key] = value;
            } else {
                paths[key] = [...new Set([...paths[key], ...value])];
            }
        }
    }

    addDependency({ name, item }: { name: string; item: DependencyItem }): void {
        const deps = this.object.compilerOptions.dependencies;
        const existing = deps[name];

        if (existing) {
            const mergedAlias = Array.from(new Set([...(existing.alias ?? []), ...(item.alias ?? [])]));
            deps[name] = {
                ...existing,
                ...item,
                alias: mergedAlias
            };
        } else {
            deps[name] = item;
        }
    }

    addDependencies(deps: Record<string, DependencyItem>): void {
        Object.entries(deps).forEach(([name, item]) => {
            this.addDependency({ name, item });
        });
    }

    public get compilerOptions(): CompilerOptions {
        return this.object.compilerOptions;
    }

    public get packageName(): string {
        return this.object.compilerOptions.package;
    }

    public get dependencies(): Record<string, DependencyItem> {
        return this.object.compilerOptions.dependencies;
    }

    public get pathSection(): Record<string, string[]> {
        return this.object.compilerOptions.paths;
    }

    public set useEmptyPackage(value: boolean) {
        this.object.compilerOptions.useEmptyPackage = value;
    }

    mergeArktsConfig(source: ArkTSConfig | undefined): void {
        if (!source) {
            return;
        }
        this.addDependencies(source.dependencies);
        this.addPathMappings(source.pathSection);

    }

    // if a dependenciesSets is like 
    // 'entry' : dependencies: ['hsp1' , 'har1']
    // 'hsp1': dependencies: ['hsp2','har2']
    // 'har1': dependencies: []
    // 'hsp2': dependencies: []
    // 'har2': dependencies: []
    // and mainPackageName is 'entry'
    // The following comments explain the situation when the function is called for the first time
    mergeArktsConfigByDependencies(dependencies: Set<string>, dependenciesSets: Map<string, Set<string>>): void {
        if (dependencies.size === 0) {
            return;
        }

        // dependencies: ['hsp1' , 'har1']
        dependencies.forEach((dependency) => {
            // dependency: 'hsp1'
            // dependencydependencies: ['hsp2','har2']
            let dependencydependencies: Set<string> = dependenciesSets.get(dependency)!;
            let arktsConfig = ArkTSConfigGenerator.getInstance().getArktsConfigByPackageName(dependency)!;
            if (dependencydependencies.size !== 0) {
                // hsp1 combines the arktsConfig of hsp2 and har2
                arktsConfig.mergeArktsConfigByDependencies(dependencydependencies, dependenciesSets);
            }

            // entry combines the arktsConfig of hsp1
            this.addDependencies(arktsConfig.dependencies);
            this.addPathMappings(arktsConfig.pathSection);
        });
    }
}

export class ArkTSConfigGenerator {
    private static instance: ArkTSConfigGenerator | undefined;
    private systemSdkPath: string;
    private stdlibAbcPath: string;

    private buildConfig: BuildConfig;

    private logger: Logger;
    private systemPathSection: Record<string, string[]>;
    private systemDependenciesSection: Record<string, DependencyItem>;
    private arktsconfigs: Map<string, ArkTSConfig>;

    private constructor(buildConfig: BuildConfig) {
        this.logger = Logger.getInstance();
        const realPandaSdkPath = safeRealpath(buildConfig.pandaSdkPath!!);
        const realBuildSdkPath = safeRealpath(buildConfig.buildSdkPath);
        this.systemSdkPath = path.resolve(realBuildSdkPath, SYSTEM_SDK_PATH_FROM_SDK);
        this.stdlibAbcPath = path.resolve(realPandaSdkPath, 'lib', 'etsstdlib.abc');
        this.buildConfig = buildConfig;

        this.systemPathSection = {}
        this.systemDependenciesSection = {};
        this.arktsconfigs = new Map();

        this.initPathInfo();
    }

    public get aliasConfig(): Record<string, Record<string, AliasConfig>> {
        return this.buildConfig.aliasConfig
    }

    public get dynamicSDKPaths(): Set<string> {
        return this.buildConfig.interopSDKPaths;
    }

    public get externalApiPaths(): string[] {
        return this.buildConfig.externalApiPaths;
    }

    public static getInstance(buildConfig?: BuildConfig): ArkTSConfigGenerator {
        if (!ArkTSConfigGenerator.instance) {
            if (!buildConfig) {
                throw new Error(
                    'buildConfig and moduleInfos is required for the first instantiation of ArkTSConfigGenerator.');
            }
            ArkTSConfigGenerator.instance = new ArkTSConfigGenerator(buildConfig);
        }
        return ArkTSConfigGenerator.instance;
    }

    public static destroyInstance(): void {
        ArkTSConfigGenerator.instance = undefined;
    }

    private generateSystemSdkPathSection(pathSection: Record<string, string[]>): void {
        function traverse(currentDir: string, relativePath: string = '', isExcludedDir: boolean = false, allowedExtensions: string[] = ['.d.ets']): void {
            const items = fs.readdirSync(currentDir);
            for (const item of items) {
                const itemPath = path.join(currentDir, item);
                const stat = fs.statSync(itemPath);
                const isAllowedFile = allowedExtensions.some(ext => item.endsWith(ext));
                if (stat.isFile() && !isAllowedFile) {
                    continue;
                }

                if (stat.isFile()) {
                    const basename = path.basename(item, '.d.ets');
                    const key = isExcludedDir ? basename : (relativePath ? `${relativePath}.${basename}` : basename);
                    pathSection[key] = [changeFileExtension(itemPath, '', '.d.ets')];
                }
                if (stat.isDirectory()) {
                    // For files under api dir excluding arkui/runtime-api dir,
                    // fill path section with `"pathFromApi.subdir.fileName" = [${absolute_path_to_file}]`;
                    // For @koalaui files under arkui/runtime-api dir,
                    // fill path section with `"fileName" = [${absolute_path_to_file}]`.
                    const isCurrentDirExcluded = path.basename(currentDir) === 'arkui' && item === 'runtime-api';
                    const newRelativePath = isCurrentDirExcluded ? '' : (relativePath ? `${relativePath}.${item}` : item);
                    traverse(path.resolve(currentDir, item), newRelativePath, isCurrentDirExcluded || isExcludedDir);
                }
            }
        }

        if (this.externalApiPaths && this.externalApiPaths.length !== 0) {
            this.externalApiPaths.forEach((sdkPath: string) => {
                fs.existsSync(sdkPath) ? traverse(sdkPath) : this.logger.printWarn(`sdk path ${sdkPath} not exist.`);
            });
        } else {
            // NOTE: to be refacotred
            // NOTE: should be removed once externalApiPaths becomes a mandatory param
            // Search openharmony sdk only, we keep them for ci compatibility.
            let apiPath: string = path.resolve(this.systemSdkPath, 'api');
            fs.existsSync(apiPath) ? traverse(apiPath) : this.logger.printWarn(`sdk path ${apiPath} not exist.`);

            let arktsPath: string = path.resolve(this.systemSdkPath, 'arkts');
            fs.existsSync(arktsPath) ? traverse(arktsPath) : this.logger.printWarn(`sdk path ${arktsPath} not exist.`);

            let kitsPath: string = path.resolve(this.systemSdkPath, 'kits');
            fs.existsSync(kitsPath) ? traverse(kitsPath) : this.logger.printWarn(`sdk path ${kitsPath} not exist.`);
        }
    }

    private getAllModuleFilesToPathSection(moduleInfo: ModuleInfo, arktsConfig: ArkTSConfig): void {
        if (!moduleInfo.staticFiles || moduleInfo.staticFiles.length === 0) {
            return;
        }
        const moduleRoot = toUnixPath(moduleInfo.moduleRootPath) + '/';
        for (const file of moduleInfo.staticFiles) {
            const unixFilePath: string = toUnixPath(file);
            if (!isSubPathOf(unixFilePath, moduleRoot)) {
                continue;
            }
            let relativePath = unixFilePath.substring(moduleRoot.length);
            // remove extension and if it is declaration file, remove .d too
            const keyWithoutExtension = relativePath.replace(/\.(?:d\.)?[^/.]+$/, '');
            const pathKey = `${moduleInfo.packageName}/${keyWithoutExtension}`;
            arktsConfig.addPathMappings({ [pathKey]: [file] });
        }
    }

    private addPathSection(moduleInfo: ModuleInfo, arktsConfig: ArkTSConfig): void {
        arktsConfig.addPathMappings(this.systemPathSection);
        // NOTE: workaround
        // NOTE: to be refactored
        if (moduleInfo.language === LANGUAGE_VERSION.ARKTS_1_1) {
            return
        }

        this.getAllFilesToPathSection(moduleInfo, arktsConfig);
        this.getAllModuleFilesToPathSection(moduleInfo, arktsConfig);
        // NOTE: is some test cases somehow packageName can be an empty string
        // NOTE: to be refactored
        if (moduleInfo.packageName) {
            arktsConfig.addPathMappings({
                [moduleInfo.packageName]: [moduleInfo.moduleRootPath]
            });
        }

        this.logger.printDebug(`Collected path section: ${JSON.stringify(arktsConfig.compilerOptions.paths, null, 1)}`)
    }

    private getDependencyKey(file: string, moduleInfo: ModuleInfo): string {
        let unixFilePath: string = file.replace(/\\/g, '/');
        return moduleInfo.packageName + '/' + unixFilePath;
    }

    private addEtsStdLibToDependencySection(arktsconfig: ArkTSConfig): void {
        const etsstdlibPackageNames: string[] = [
            'std/core',
            'std/math',
            'std/math/consts',
            'std/containers',
            'std/interop/js',
            'std/time',
            'std/debug',
            'std/debug/concurrency',
            'std/testing',
            'std/concurrency',
            'std/annotations',
            'std/interop',
            'escompat'
        ]

        etsstdlibPackageNames.forEach((currentName: string) => {
            const etsstdlibItem: DependencyItem = { language: 'ets', path: this.stdlibAbcPath, ohmUrl: currentName };
            arktsconfig.addDependency({ name: etsstdlibItem.ohmUrl, item: etsstdlibItem });
        });
    }

    private addDependenciesSection(moduleInfo: ModuleInfo, arktsconfig: ArkTSConfig): void {
        moduleInfo.dynamicDependencyModules.forEach((depModuleInfo: ModuleInfo) => {
            if (!depModuleInfo.declFilesPath || !fs.existsSync(depModuleInfo.declFilesPath)) {
                this.logger.printWarn(`Module ${moduleInfo.packageName} depends on dynamic module ${depModuleInfo.packageName}` +
                    `, but decl file not found on path ${depModuleInfo.declFilesPath}`);
                return;
            }

            const declFilesObject = JSON.parse(fs.readFileSync(depModuleInfo.declFilesPath, 'utf-8'));
            const files = declFilesObject.files;

            Object.keys(files).forEach((file: string) => {
                const dependencyKey: string = this.getDependencyKey(file, depModuleInfo);
                const depItem: DependencyItem = {
                    language: 'js',
                    path: files[file].declPath,
                    sourceFilePath: files[file].filePath,
                    ohmUrl: files[file].ohmUrl
                };

                arktsconfig.addDependency({
                    name: dependencyKey,
                    item: depItem
                });

                // NOTE: workaround
                // NOTE: to be refactored
                const absFilePath: string = file;
                const entryFileWithoutExtension: string = changeFileExtension(depModuleInfo.entryFile, '');

                if (absFilePath === entryFileWithoutExtension) {
                    arktsconfig.addDependency({
                        name: depModuleInfo.packageName,
                        item: depItem
                    });
                }
            });
        });
        arktsconfig.addDependencies(this.systemDependenciesSection);
    }

    public generateArkTSConfigFile(moduleInfo: ModuleInfo, enableDeclgenEts2Ts: boolean): ArkTSConfig {
        if (!moduleInfo.sourceRoots || moduleInfo.sourceRoots.length === 0) {
            throw new DriverError(
                LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_SOURCEROOTS_NOT_SET_FAIL,
                    `SourceRoots not set for module ${moduleInfo.packageName}.`
                )
            );
        }
        let arktsConfig: ArkTSConfig = new ArkTSConfig(moduleInfo, this.buildConfig.cachePath, this.buildConfig.projectRootPath);
        this.arktsconfigs.set(moduleInfo.packageName, arktsConfig);
        this.addPathSection(moduleInfo, arktsConfig);

        if (!enableDeclgenEts2Ts) {
            this.addDependenciesSection(moduleInfo, arktsConfig);
        }
        this.addEtsStdLibToDependencySection(arktsConfig);

        this.processAlias(arktsConfig);

        if (this.buildConfig.frameworkMode) {
            arktsConfig.useEmptyPackage = this.buildConfig.useEmptyPackage ?? false;
        }

        ensurePathExists(moduleInfo.arktsConfigFile);

        this.logger.printDebug(`arktsconfig for ${moduleInfo.packageName}:\n${JSON.stringify(arktsConfig, null, 1)}`)
        return arktsConfig;
    }

    private processAlias(arktsconfigs: ArkTSConfig): void {
        const aliasForPkg = this.aliasConfig?.[arktsconfigs.packageName];
        if (!aliasForPkg) {
            return;
        }
        for (const [aliasName, aliasConfig] of Object.entries(aliasForPkg)) {
            if (aliasConfig.isStatic) {
                continue;
            }
            if (aliasConfig.originalAPIName.startsWith('@kit')) {
                this.processStaticAlias(aliasName, aliasConfig, arktsconfigs);
            } else {
                this.processDynamicAlias(aliasName, aliasConfig, arktsconfigs);
            }
        }
    }

    private traverseDependencies(
        currentDir: string,
        relativePath: string,
        isExcludedDir: boolean,
        dependencySection: Record<string, DependencyItem>,
        prefix: string = ''
    ): void {
        const allowedExtensions = ['.d.ets'];
        const items = fs.readdirSync(currentDir);

        for (const item of items) {
            const itemPath = path.join(currentDir, item);
            const stat = fs.statSync(itemPath);

            if (stat.isFile()) {
                if (this.isAllowedExtension(item, allowedExtensions)) {
                    this.processDynamicFile({
                        filePath: itemPath,
                        fileName: item,
                        relativePath,
                        isExcludedDir,
                        dependencySection,
                        prefix
                    });
                }
                continue;
            }

            if (stat.isDirectory()) {
                const isRuntimeAPI = path.basename(currentDir) === 'arkui' && item === 'runtime-api';
                const newRelativePath = isRuntimeAPI
                    ? ''
                    : (relativePath ? `${relativePath}/${item}` : item);

                this.traverseDependencies(
                    path.resolve(currentDir, item),
                    newRelativePath,
                    isExcludedDir || isRuntimeAPI,
                    dependencySection,
                    prefix
                );
            }
        }
    }

    private isAllowedExtension(fileName: string, allowedExtensions: string[]): boolean {
        return allowedExtensions.some(ext => fileName.endsWith(ext));
    }

    private isValidAPIFile(fileName: string): boolean {
        const pattern = new RegExp(`^@(${sdkConfigPrefix})\\..+\\.d\\.ets$`, 'i');
        return pattern.test(fileName);
    }

    private buildDynamicKey(
        baseName: string,
        relativePath: string,
        isExcludedDir: boolean,
        separator: string = '.'
    ): string {
        return isExcludedDir
            ? baseName
            : (relativePath ? `${relativePath}${separator}${baseName}` : baseName);
    }


    private processDynamicFile(ctx: DynamicFileContext): void {
        const {
            filePath,
            fileName,
            relativePath,
            isExcludedDir,
            dependencySection,
            prefix = ''
        } = ctx;
        let separator = '.'
        if (!this.isValidAPIFile(fileName)) {
            separator = '/'
        }

        const baseName = path.basename(fileName, '.d.ets');
        const normalizedRelativePath = relativePath.replace(/\//g, separator);
        const key = this.buildDynamicKey(baseName, normalizedRelativePath, isExcludedDir, separator);

        dependencySection[prefix + key] = {
            language: 'js',
            path: filePath,
            ohmUrl: getOhmurlByApi(baseName),
            alias: [key]
        };
    }

    private processStaticAlias(
        aliasName: string,
        aliasConfig: AliasConfig,
        arktsConfig: ArkTSConfig
    ): void {
        const declPath = getInteropFilePathByApi(aliasConfig.originalAPIName, this.dynamicSDKPaths);
        if (!declPath) {
            return;
        }

        arktsConfig.addPathMappings({
            [aliasName]: [declPath]
        });
    }

    private processDynamicAlias(
        aliasName: string,
        aliasConfig: AliasConfig,
        arktsConfig: ArkTSConfig
    ): void {
        const originalName = aliasConfig.originalAPIName;
        const declPath = getInteropFilePathByApi(originalName, this.dynamicSDKPaths);

        if (declPath === '') {
            return;
        }

        if (!fs.existsSync(declPath)) {
            throw new DriverError(
                LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_INTEROP_SDK_NOT_FIND,
                    `Interop SDK File Not Exist: ${declPath}`
                )
            );
        }

        arktsConfig.addDependency({
            name: DYNAMIC_PREFIX + originalName,
            item: {
                language: 'js',
                path: declPath,
                ohmUrl: getOhmurlByApi(originalName),
                alias: [aliasName]
            }
        });
    }

    private getAllFilesToPathSection(
        moduleInfo: ModuleInfo,
        arktsConfig: ArkTSConfig
    ): void {
        const moduleRoot = toUnixPath(moduleInfo.moduleRootPath) + '/';

        for (const file of this.buildConfig.compileFiles) {
            const unixFilePath: string = toUnixPath(file);

            if (!isSubPathOf(unixFilePath, moduleRoot)) {
                continue;
            }

            let relativePath = unixFilePath.substring(moduleRoot.length);
            // remove extension and if it is declaration file, remove .d too
            const keyWithoutExtension = relativePath.replace(/\.(?:d\.)?[^/.]+$/, '');

            const pathKey = `${moduleInfo.packageName}/${keyWithoutExtension}`;
            arktsConfig.addPathMappings({ [pathKey]: [file] });
        }
    }

    private initPathInfo(): void {
        this.generateSystemSdkPathSection(this.systemPathSection);
        this.generateSystemSdkDependenciesSection(this.systemDependenciesSection);
        if (this.buildConfig.paths) {
            Object.entries(this.buildConfig.paths).map(([key, value]) => {
                this.systemPathSection[key] = value
            });
        }
    }

    private generateSystemSdkDependenciesSection(dependenciesSection: Record<string, DependencyItem>): void {
        this.dynamicSDKPaths.forEach(basePath => {
            if (basePath.includes(KITS)) {
                return;
            }
            if (!fs.existsSync(basePath)) {
                throw new DriverError(
                    LogDataFactory.newInstance(
                        ErrorCode.BUILDSYSTEM_ALIAS_MODULE_PATH_NOT_EXIST,
                        `alias module ${basePath} not exist.`
                    )
                );
            }
            if (basePath.includes(COMPONENT)) {
                this.traverseDependencies(basePath, '', false, dependenciesSection, 'component/');
            } else {
                this.traverseDependencies(basePath, '', false, dependenciesSection, DYNAMIC_PREFIX);
            }
        });
    }

    public getArktsConfigByPackageName(packageName: string): ArkTSConfig | undefined {
        return this.arktsconfigs.get(packageName);
    }
}
