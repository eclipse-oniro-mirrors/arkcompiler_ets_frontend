/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

import * as fs from 'fs';
import * as path from 'path';
import { Logger } from '../logger';
import { AbilityConfig, BuildConfig, CardFormConfig, EntryConfig, ModuleJsonConfig, StartupConfig, StartupTask } from '../types';
import { API, ARKTS, KITS } from '../pre_define';
import { getProfilePath, toUnixPath } from '../util/utils';

export class SdkApiCollect {

    public readAllFiles(dirPath: string): string[] {
        let files: string[] = [];
        const entries = fs.readdirSync(dirPath, { withFileTypes: true });
        for (const entry of entries) {
            const fullPath = path.join(dirPath, entry.name);
            if (entry.isDirectory()) {
                files = [...files, ...this.readAllFiles(fullPath)];
            } else if (entry.isFile() && (entry.name.endsWith('.d.ets') || entry.name.endsWith('.d.ts'))) {
                files.push(fullPath);
            }
        }
        return files;
    }

    public convertSingleFile(apiPath: string, filePath: string): string {
        let namespacePath = '';
        if (filePath.startsWith(apiPath)) {
            let relativePath = path.relative(apiPath, filePath);
            relativePath = relativePath.replace(/\.(d\.ets|d\.ts)$/, '');
            namespacePath = relativePath.replace(/[\/\\]/g, '.');
        } else {
            namespacePath = path.relative(apiPath, filePath);
            namespacePath = namespacePath.replace(/\.(d\.ets|d\.ts)$/, '');
            namespacePath = namespacePath.replace(/[\/\\]/g, '.');
        }
        if (namespacePath.startsWith('@')) {
            namespacePath = namespacePath.replace('@', '*');
        }
        let result = `class ${namespacePath}.* {*;}`;
        let extendsDes = `class ** extends ${namespacePath}.* {*;}`;
        return `${result}\n${extendsDes}`;
    }
}

export function collectSdkApiWhitelist(apiPath: string): string {
    const logger: Logger = Logger.getInstance();
    if (!fs.existsSync(apiPath)) {
        logger.printWarn(`Directory ${apiPath} does not exist`);
        return '';
    }
    let allContent: string[] = [];
    const collect = new SdkApiCollect();
    const files = collect.readAllFiles(apiPath);
    for (let i = 0; i < files.length; i++) {
        const file = files[i];
        try {
            const result = collect.convertSingleFile(apiPath, file);
            allContent.push(result);
        } catch (error) {
            logger.printWarn(`Error processing file ${file}`);
        }
    }
    return allContent.join('\n');
}

export function getSdkApiList(buildConfig: BuildConfig): string[] {
    const buildSdkPath = buildConfig.buildSdkPath;
    if (!buildSdkPath) {
        return [];
    }
    let sdkApiList: string[] = [];
    const kitsPath = path.join(buildSdkPath, KITS);
    if (fs.existsSync(kitsPath)) {
        sdkApiList.push(kitsPath);
    }
    const apiPath = path.join(buildSdkPath, API);
    if (fs.existsSync(apiPath)) {
        sdkApiList.push(apiPath);
    }
    const arktsApiPath = path.join(buildSdkPath, ARKTS);
    if (fs.existsSync(arktsApiPath)) {
        sdkApiList.push(arktsApiPath);
    }
    const hmsApiPath = path.resolve(buildSdkPath, '../../../../hms/ets/static/', API);
    if (fs.existsSync(hmsApiPath)) {
        sdkApiList.push(hmsApiPath);
    }
    const hmsKitsPath = path.resolve(buildSdkPath, '../../../../hms/ets/static/', KITS);
    if (fs.existsSync(hmsKitsPath)) {
        sdkApiList.push(hmsKitsPath);
    }
    return sdkApiList
}

export function getDefaultKeepFileNameWhitelist(buildConfig: BuildConfig): string[] {
    let result: string[] = [];
    let abilityPages: string[] = [];
    if (buildConfig.aceModuleJsonPath && fs.existsSync(buildConfig.aceModuleJsonPath)) {
        const moduleJson = JSON.parse(fs.readFileSync(buildConfig.aceModuleJsonPath).toString());
        if (moduleJson && moduleJson.module) {
            result.push(...setStartupPages(buildConfig, moduleJson));
            abilityPages.push(...setEntryPages(buildConfig, moduleJson));
            abilityPages.push(...readAbilityPages(buildConfig, moduleJson));
        }
        result.push(...readAbilityFile(buildConfig, moduleJson, abilityPages));

    }
    return result;
}
function setStartupPages(buildConfig: BuildConfig, moduleJson: ModuleJsonConfig): string[] {
    if (!moduleJson?.module?.appStartup || !buildConfig.aceProfilePath) {
        return [];
    }
    const startupFilePath = path.resolve(buildConfig.aceProfilePath, getProfilePath(moduleJson.module.appStartup));
    try {
        return processStartupFile(startupFilePath);
    } catch (error) {
        Logger.getInstance().printWarn(`BUIDERROR: the ${startupFilePath} file format is invalid.`);
        return [];
    }
}

function processStartupFile(startupFilePath: string): string[] {
    if (!startupFilePath || !fs.existsSync(startupFilePath)) {
        return [];
    }
    const result: string[] = [];
    try {
        const startupConfig: StartupConfig = JSON.parse(fs.readFileSync(startupFilePath).toString());
        if (!startupConfig) {
            return [];
        }
        if (startupConfig.configEntry) {
            result.push(startupConfig.configEntry);
        }
        startupConfig.startupTasks?.forEach((task: StartupTask) => {
            if (task.srcEntry) {
                result.push(task.srcEntry);
            }
        });
    } catch (e) {
        Logger.getInstance().printWarn(`BUIDERROR: the ${startupFilePath} file format is invalid.`);
        return [];
    }

    return result;
}

function setEntryPages(buildConfig: BuildConfig, moduleJson: ModuleJsonConfig): string[] {
    const abilityPages: string[] = [];
    if (moduleJson?.module?.pages && buildConfig.aceProfilePath) {
        const entryFilePath = path.resolve(buildConfig.aceProfilePath, getProfilePath(moduleJson.module.pages));
        try {
            const entryConfig: EntryConfig = JSON.parse(fs.readFileSync(entryFilePath).toString());
            if (!entryConfig) {
                return [];
            }
            entryConfig.src?.forEach((src: string) => abilityPages.push(src));
        } catch (e) {
            Logger.getInstance().printWarn(`BUIDERROR: the ${entryFilePath} file format is invalid.`);
            return [];
        }
    }
    return abilityPages;
}

function readAbilityPages(buildConfig: BuildConfig, moduleJson: ModuleJsonConfig): string[] {
    const abilityPages: string[] = [];
    const moduleSrcEntrance = moduleJson.module?.srcEntrance;
    const moduleSrcEntry = moduleJson.module?.srcEntry;
    if (moduleSrcEntrance) {
        abilityPages.push(moduleSrcEntrance);
    } else if (moduleSrcEntry) {
        abilityPages.push(moduleSrcEntry);
    }
    moduleJson.module?.abilities?.forEach((ability: AbilityConfig) => {
        if (ability.srcEntry) {
            abilityPages.push(ability.srcEntry);
        } else if (ability.srcEntrance) {
            abilityPages.push(ability.srcEntrance);
        }
    });
    abilityPages.push(...readExtensionAbilitiesPages(buildConfig, moduleJson));
    return abilityPages;
}

function processExtensionAbility(buildConfig: BuildConfig, ability: AbilityConfig): string[] {
    const pages: string[] = [];
    if (ability.srcEntry) {
        pages.push(ability.srcEntry);
    } else if (ability.srcEntrance) {
        pages.push(ability.srcEntrance);
    }
    if (ability.metadata) {
        ability.metadata.forEach(md => {
            if (md.resource) {
                pages.push(...readCardResource(buildConfig, md.resource));
            }
        });
    }
    return pages;
}

function readExtensionAbilitiesPages(buildConfig: BuildConfig, moduleJson: ModuleJsonConfig): string[] {
    if (!moduleJson?.module) {
        return [];
    }
    return (moduleJson.module?.extensionAbilities || [])
        .flatMap(ability => processExtensionAbility(buildConfig, ability));
}

function readCardResource(buildConfig: BuildConfig, resource: string): string[] {
    if (!buildConfig.aceProfilePath) {
        return [];
    }
    const modulePagePath = path.resolve(buildConfig.aceProfilePath, getProfilePath(resource));
    if (!fs.existsSync(modulePagePath)) {
        return [];
    }
    const result: string[] = [];
    try {
        const cardConfig = JSON.parse(fs.readFileSync(modulePagePath, 'utf-8'));
        if (!cardConfig.forms || !Array.isArray(cardConfig.forms)) {
            return result;
        }
        result.push(...readCardForms(cardConfig.forms));
    } catch (error) {
        Logger.getInstance().printWarn(`Failed to parse card config file: ${modulePagePath}`);
    }
    return result;
}
function readCardForms(forms: Array<CardFormConfig>): string[] {
    const result: string[] = [];
    forms.forEach((form: CardFormConfig) => {
        const isUIForm = (form.type && form.type === 'eTS') || (form.uiSyntax && form.uiSyntax === 'arkts');
        if (isUIForm && form.src) {
            result.push(form.src);
        }
    });
    return result;
}

function readAbilityFile(buildConfig: BuildConfig, moduleJson: ModuleJsonConfig, abilityPages: string[]): string[] {
    let result: string[] = [];
    abilityPages.forEach(abilityPath => {
        let projectAbilityPath: string = '';
        let entryPageKey: string = '';
        if (path.isAbsolute(abilityPath)) {
            projectAbilityPath = abilityPath;
            entryPageKey = path.relative(buildConfig.projectRootPath, projectAbilityPath);
        } else {
            const parts = readAbilityComplexParts(buildConfig, moduleJson, abilityPath);
            entryPageKey = parts.entryPageKey;
            projectAbilityPath = parts.projectAbilityPath;
        }
        result.push(...setAbilityFile(moduleJson, entryPageKey, projectAbilityPath));
    });
    return result;
}

function readAbilityComplexParts(buildConfig: BuildConfig, moduleJson: ModuleJsonConfig,
    abilityPath: string): { entryPageKey: string, projectAbilityPath: string } {
    if (!buildConfig.aceModuleRoot) {
        return { entryPageKey: '', projectAbilityPath: '' };
    }   
    let projectAbilityPath: string = '';
    let entryPageKey: string = '';
    const moduleRootRelative = toUnixPath(path.relative(buildConfig.projectRootPath, buildConfig.aceModuleRoot));
    if (moduleJson.module?.packageName && moduleJson.module.name && abilityPath.startsWith(moduleJson.module.packageName)) {
        abilityPath = abilityPath.replace(moduleJson.module.packageName, moduleJson.module.name);
    }
    if (abilityPath.startsWith(moduleRootRelative + '/') || abilityPath === moduleRootRelative) {
        projectAbilityPath = path.resolve(buildConfig.projectRootPath, abilityPath);
        entryPageKey = abilityPath;
    } else if (abilityPath.startsWith('./')) {
        projectAbilityPath = path.resolve(buildConfig.aceModuleRoot, '../', abilityPath);
        entryPageKey = path.relative(buildConfig.projectRootPath, projectAbilityPath);
    } else {
        projectAbilityPath = path.resolve(buildConfig.aceModuleRoot, abilityPath);
        entryPageKey = path.relative(buildConfig.projectRootPath, projectAbilityPath);
    }
    return { entryPageKey, projectAbilityPath };
}

function setAbilityFile(moduleJson: ModuleJsonConfig, entryPageKey: string, projectAbilityPath: string): string[] {
    let result: string[] = [];
    entryPageKey = toUnixPath(entryPageKey.replace(/^\.\/ets\//, './').replace(/\.ts$/, '').replace(/\.ets$/, ''));
    if (fs.existsSync(projectAbilityPath)) {
        result.push(entryPageKey);
    } else {
        const etsPath = projectAbilityPath + '.ets';
        const tsPath = projectAbilityPath + '.ts';
        if (fs.existsSync(etsPath) || fs.existsSync(tsPath)) {
            if (moduleJson.module?.packageName && moduleJson.module.name && entryPageKey.startsWith(moduleJson.module.name)){
                entryPageKey = entryPageKey.replace(moduleJson.module.name, moduleJson.module.packageName);
            }
            result.push(entryPageKey);
        } else {
            const projectAbilityDeclFilePath = projectAbilityPath.replace(/\.ts$/, '.d.ts').replace(/\.ets$/, '.d.ets');
            Logger.getInstance().printWarn(`srcEntry file '${projectAbilityDeclFilePath.replace(/\\/g, '/')}' does not exist`);
        }
    }
    return result;
}