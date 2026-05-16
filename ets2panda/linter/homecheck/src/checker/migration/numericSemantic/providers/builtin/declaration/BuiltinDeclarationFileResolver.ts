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

    public getDynBuiltinDeclarationFiles(): string[] {
        return this.getSdkPathDynBuiltinDeclarationFiles();
    }

    public getDynBuiltinDeclarationFilesFromSdkPath(sdkPath: string, targetESVersion?: string): string[] {
        const resolvedTargetESVersion = this.getTargetESVersion(targetESVersion);
        const libDirs = this.getDynBuiltinDeclarationLibDirsFromSdkPath(sdkPath);
        const files: string[] = [];
        for (const libDir of libDirs) {
            files.push(...this.getDynBuiltinDeclarationFilesFromLibDir(libDir, resolvedTargetESVersion));
        }
        return [...new Set(files)];
    }

    private getSdkPathDynBuiltinDeclarationFiles(): string[] {
        const files: string[] = [];
        for (const sdkPath of this.getProjectSdkPaths()) {
            files.push(...this.getDynBuiltinDeclarationFilesFromSdkPath(sdkPath, this.ruleOptions?.targetESVersion));
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

    private getDynBuiltinDeclarationFilesFromLibDir(libDir: string, targetESVersion: string): string[] {
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
        return [...files].filter(filePath => this.isDynBuiltinDeclarationFile(filePath));
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

    private isExistingDirectory(dir: string): boolean {
        try {
            return fs.existsSync(dir) && fs.statSync(dir).isDirectory();
        } catch (e) {
            return false;
        }
    }
}
