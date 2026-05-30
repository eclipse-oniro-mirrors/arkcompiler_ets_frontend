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

import fs from 'fs';
import path from 'path';
import { Scene, ts } from 'arkanalyzer/lib';
import Logger, { LOG_MODULE_TYPE } from 'arkanalyzer/lib/utils/logger';
import {
    BUILD_PROFILE_JSON5,
    BUILTIN_ES_VERSION_ENTRY_FILES,
    DEFAULT_BUILTIN_TARGET_ES_VERSION,
    RuleOptions,
} from '../../../core/NumericSemanticTypes';

const logger = Logger.getLogger(LOG_MODULE_TYPE.HOMECHECK, 'BuiltinDeclarationFileResolver');

export class BuiltinDeclarationFileResolver {
    constructor(private scene?: Scene, private ruleOptions?: RuleOptions) {}

    public getStaBuiltinDeclarationFiles(): string[] {
        const dirs = this.getStaBuiltinDeclarationDirs();
        const files: string[] = [];
        for (const dir of dirs) {
            files.push(...this.getFilesWithExtension(dir, '.d.ts'));
            files.push(...this.getFilesWithExtension(dir, '.d.ets'));
            files.push(...this.getFilesWithExtension(dir, '.ets'));
        }
        return [...new Set(files)];
    }

    public getDynBuiltinDeclarationFiles(staticBuiltinDeclarationFiles: string[] = []): string[] {
        return this.getSdkPathDynBuiltinDeclarationFiles(this.collectStaticBuiltinTypeNames(staticBuiltinDeclarationFiles));
    }

    public getDynBuiltinDeclarationFilesFromSdkPath(
        sdkPath: string,
        targetESVersion?: string,
        staticBuiltinTypeNames: Set<string> = new Set<string>()
    ): string[] {
        const resolvedTargetESVersion = this.getTargetESVersion(targetESVersion);
        const libDirs = this.getDynBuiltinDeclarationLibDirsFromSdkPath(sdkPath);
        const files: string[] = [];
        for (const libDir of libDirs) {
            files.push(...this.getDynBuiltinDeclarationFilesFromLibDir(libDir, resolvedTargetESVersion, staticBuiltinTypeNames));
        }
        if (files.length > 0) {
            return [...new Set(files)];
        }
        return this.getInternalDynBuiltinDeclarationFiles(resolvedTargetESVersion, staticBuiltinTypeNames);
    }

    private getSdkPathDynBuiltinDeclarationFiles(staticBuiltinTypeNames: Set<string>): string[] {
        const files: string[] = [];
        for (const sdkPath of this.getProjectSdkPaths()) {
            files.push(...this.getDynBuiltinDeclarationFilesFromSdkPath(
                sdkPath,
                this.ruleOptions?.targetESVersion,
                staticBuiltinTypeNames
            ));
        }
        return [...new Set(files)];
    }

    private getProjectSdkPaths(): string[] {
        if (!this.scene) {
            return [];
        }
        return [...this.scene.getProjectSdkMap().values()]
            .map(sdk => sdk.path)
            .filter(sdkPath => sdkPath.length > 0);
    }

    private getTargetESVersion(targetESVersion?: string): string {
        if (targetESVersion && BUILTIN_ES_VERSION_ENTRY_FILES.has(targetESVersion)) {
            return targetESVersion;
        }

        const projectDir = this.scene?.getRealProjectDir();
        if (!projectDir) {
            return DEFAULT_BUILTIN_TARGET_ES_VERSION;
        }

        const buildProfilePath = path.join(projectDir, BUILD_PROFILE_JSON5);
        try {
            const text = fs.readFileSync(buildProfilePath, 'utf8');
            const match = text.match(/["']?targetESVersion["']?\s*:\s*["']([^"']+)["']/u);
            if (match && BUILTIN_ES_VERSION_ENTRY_FILES.has(match[1])) {
                return match[1];
            }
        } catch (e) {
            logger.debug(`Failed to read targetESVersion from ${buildProfilePath}: ${e}`);
        }

        return DEFAULT_BUILTIN_TARGET_ES_VERSION;
    }

    private getDynBuiltinDeclarationFilesFromLibDir(
        libDir: string,
        targetESVersion: string,
        staticBuiltinTypeNames: Set<string>
    ): string[] {
        const entryFile = BUILTIN_ES_VERSION_ENTRY_FILES.get(targetESVersion) ?? BUILTIN_ES_VERSION_ENTRY_FILES.get(DEFAULT_BUILTIN_TARGET_ES_VERSION);
        if (!entryFile) {
            return [];
        }
        const entryPath = path.join(libDir, entryFile);
        if (!fs.existsSync(entryPath)) {
            return [];
        }

        const files = new Set<string>();
        this.collectDynBuiltinLibReferences(entryPath, files);
        this.collectDynBuiltinLibFilesForStaticTypes(libDir, staticBuiltinTypeNames, files);
        return [...files].filter(filePath => this.isDynBuiltinDeclarationFile(filePath));
    }

    private getInternalDynBuiltinDeclarationFiles(targetESVersion: string, staticBuiltinTypeNames: Set<string>): string[] {
        const files: string[] = [];
        for (const libDir of this.getInternalDynBuiltinDeclarationLibDirs()) {
            files.push(...this.getDynBuiltinDeclarationFilesFromLibDir(libDir, targetESVersion, staticBuiltinTypeNames));
        }
        return [...new Set(files)];
    }

    private getInternalDynBuiltinDeclarationLibDirs(): string[] {
        const dirs = [
            path.resolve(__dirname, '../../../../../../../resources/internalSdk/@internal'),
            path.resolve(process.cwd(), 'node_modules/homecheck/resources/internalSdk/@internal'),
            path.resolve(process.cwd(), 'homecheck/resources/internalSdk/@internal'),
        ];
        return [...new Set(dirs)].filter(dir => this.isExistingDirectory(dir));
    }

    private collectDynBuiltinLibReferences(filePath: string, files: Set<string>): void {
        if (files.has(filePath) || !fs.existsSync(filePath)) {
            return;
        }

        const sourceFile = ts.createSourceFile(filePath, fs.readFileSync(filePath, 'utf8'), ts.ScriptTarget.Latest);
        sourceFile.libReferenceDirectives.forEach(ref => {
            this.collectDynBuiltinLibReferences(path.join(path.dirname(filePath), `lib.${ref.fileName}.d.ts`), files);
        });
        files.add(filePath);
    }

    private collectDynBuiltinLibFilesForStaticTypes(libDir: string, staticBuiltinTypeNames: Set<string>, files: Set<string>): void {
        if (staticBuiltinTypeNames.size === 0) {
            return;
        }

        for (const fileName of this.readDirectoryFiles(libDir)) {
            const filePath = path.join(libDir, fileName);
            if (files.has(filePath) || !this.isDynBuiltinDeclarationFile(filePath)) {
                continue;
            }
            if (this.containsBuiltinTypeDeclaration(filePath, staticBuiltinTypeNames)) {
                this.collectDynBuiltinLibReferences(filePath, files);
            }
        }
    }

    private collectStaticBuiltinTypeNames(filePaths: string[]): Set<string> {
        const names = new Set<string>();
        filePaths.forEach(filePath => {
            const text = this.readFileText(filePath);
            if (!text) {
                return;
            }
            this.collectDeclaredTypeNames(text, names);
        });
        return names;
    }

    private containsBuiltinTypeDeclaration(filePath: string, staticBuiltinTypeNames: Set<string>): boolean {
        const names = new Set<string>();
        const text = this.readFileText(filePath);
        if (!text) {
            return false;
        }
        this.collectDeclaredTypeNames(text, names);
        return [...names].some(name => staticBuiltinTypeNames.has(name));
    }

    private collectDeclaredTypeNames(text: string, names: Set<string>): void {
        const declarationPattern = /\b(?:export\s+)?(?:declare\s+)?(?:abstract\s+)?(?:class|interface)\s+([A-Za-z_$][\w$]*)/gu;
        let declarationMatch = declarationPattern.exec(text);
        while (declarationMatch !== null) {
            names.add(declarationMatch[1]);
            declarationMatch = declarationPattern.exec(text);
        }

        const variablePattern = /\b(?:export\s+)?declare\s+var\s+([A-Za-z_$][\w$]*)/gu;
        let variableMatch = variablePattern.exec(text);
        while (variableMatch !== null) {
            names.add(variableMatch[1]);
            variableMatch = variablePattern.exec(text);
        }
    }

    private getDynBuiltinDeclarationLibDirsFromSdkPath(sdkPath: string): string[] {
        const sdkPathWithSlash = sdkPath.replace(/\\/g, '/');
        const dir = path.join(
            this.getBuiltinSdkRoot(sdkPathWithSlash),
            'ets',
            'dynamic',
            'build-tools',
            'ets-loader',
            'node_modules',
            'typescript',
            'lib'
        );
        return this.isExistingDirectory(dir) ? [dir] : [];
    }

    private isDynBuiltinDeclarationFile(filePath: string): boolean {
        return path.basename(filePath).startsWith('lib.') && filePath.endsWith('.d.ts');
    }

    private getStaBuiltinDeclarationDirs(): string[] {
        const dirs: string[] = [];
        for (const sdk of this.scene?.getProjectSdkMap().values() ?? []) {
            dirs.push(...this.getStaBuiltinDeclarationDirsFromSdkPath(sdk.path));
        }
        for (const sdk of this.ruleOptions?.ets2Sdks ?? []) {
            dirs.push(...this.getStaBuiltinDeclarationDirsFromSdkPath(sdk.path));
        }
        return [...new Set(dirs)];
    }

    private getStaBuiltinDeclarationDirsFromSdkPath(sdkPath: string): string[] {
        const sdkPathWithSlash = sdkPath.replace(/\\/g, '/');
        const dir = path.join(this.getBuiltinSdkRoot(sdkPathWithSlash), 'ets', 'static', 'arkts', 'builtin', 'static');
        return this.isExistingDirectory(dir) ? [dir] : [];
    }

    private getBuiltinSdkRoot(sdkPathWithSlash: string): string {
        const dynamicIndex = sdkPathWithSlash.indexOf('/ets/dynamic');
        if (dynamicIndex !== -1) {
            return sdkPathWithSlash.substring(0, dynamicIndex);
        }

        const staticIndex = sdkPathWithSlash.indexOf('/ets/static');
        if (staticIndex !== -1) {
            return sdkPathWithSlash.substring(0, staticIndex);
        }

        if (sdkPathWithSlash.endsWith('/ets')) {
            return sdkPathWithSlash.substring(0, sdkPathWithSlash.length - '/ets'.length);
        }

        return sdkPathWithSlash;
    }

    private getFilesWithExtension(dir: string, extension: string): string[] {
        try {
            return fs.readdirSync(dir)
                .filter(fileName => fileName.endsWith(extension))
                .map(fileName => path.join(dir, fileName))
                .filter(filePath => fs.statSync(filePath).isFile());
        } catch (e) {
            logger.debug(`Failed to read builtin declaration dir ${dir}: ${e}`);
            return [];
        }
    }

    private readDirectoryFiles(dir: string): string[] {
        try {
            return fs.readdirSync(dir);
        } catch (e) {
            logger.debug(`Failed to read directory ${dir}: ${e}`);
            return [];
        }
    }

    private readFileText(filePath: string): string | null {
        try {
            return fs.readFileSync(filePath, 'utf8');
        } catch (e) {
            logger.debug(`Failed to read file ${filePath}: ${e}`);
            return null;
        }
    }

    private isExistingDirectory(dir: string): boolean {
        try {
            return fs.existsSync(dir) && fs.statSync(dir).isDirectory();
        } catch (e) {
            return false;
        }
    }
}
