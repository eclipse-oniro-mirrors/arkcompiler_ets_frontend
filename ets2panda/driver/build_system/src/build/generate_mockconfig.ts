/**
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
import { BuildConfig, DependencyModuleConfig, MockParams } from '../types';
import {
    LogDataFactory
} from '../logger';
import {
    DriverError,
    ErrorCode
} from '../util/error';
import {
    isFirstLineUseStatic
} from '../util/utils';
import {
    ETS_SUFFIX
} from '../pre_define';

interface MockConfigInfo {
    [key: string]: { source: string };
}

export class MockConfigGenerator {
    private buildConfig: BuildConfig;
    private readonly systemApiPathSection: Record<string, string[]>;
    private mockConfigInfo: MockConfigInfo = {};

    public constructor(buildConfig: BuildConfig, systemApiPathSection: Record<string, string[]>) {
        this.buildConfig = buildConfig;
        this.systemApiPathSection = systemApiPathSection;
    }

    public get mockParams(): MockParams {
        return this.buildConfig.mockParams!;
    }

    public collectMockConfigInfo(): void {
        const rawConfig: MockConfigInfo =
            require('json5').parse(fs.readFileSync(this.mockParams.mockConfigPath, 'utf-8'));
        const sourceBasePath = this.buildConfig.moduleRootPath;
        for (const key of Object.keys(rawConfig)) {
            const entry = rawConfig[key];
            this.validateEntry(key, entry);
            const resolvedKey = this.validateKeyFile(key, sourceBasePath);
            const resolvedSource = this.validateMockConfigSource(entry.source, sourceBasePath);
            this.mockConfigInfo[resolvedKey] = { source: resolvedSource };
        }
    }

    private validateEntry(key: string, entry: unknown): void {
        if (!entry || typeof entry !== 'object' || typeof (entry as { source?: unknown }).source !== 'string') {
            throw new DriverError(
                LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_INCORRECT_MOCK_CONFIG,
                    'Incorrect mock configuration.',
                    `Mock config entry for "${key}" is invalid. Each entry must be an object with a "source" string field.`
                )
            );
        }
    }

    private validateMockConfigSource(source: string, sourceBasePath: string): string {
        if (!source) {
            return source;
        }

        const mockFullPath = this.resolveAndVerifyPath(sourceBasePath, source);
        if (!mockFullPath) {
            throw new DriverError(
                LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_INCORRECT_MOCK_CONFIG,
                    'Incorrect mock configuration.',
                    `The source file for "${source}" is invalid. ` +
                    `Source file not found under ${sourceBasePath}.`
                )
            );
        }

        if (!isFirstLineUseStatic(mockFullPath)) {
            throw new DriverError(
                LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_INCORRECT_MOCK_CONFIG,
                    'Incorrect mock configuration.',
                    `The mock file for "${source}" is invalid. ` +
                    `The first line of the file must be 'use static'. ` +
                    `File path: ${mockFullPath}`
                )
            );
        }
        return mockFullPath;
    }

    private validateKeyFile(key: string, sourceBasePath: string): string {
        if (key.startsWith('@ohos') || key.startsWith('@arkts') || key.startsWith('@kit')) {
            return this.validateSystemApi(key);
        }

        const dep = this.buildConfig.dependencyModuleList?.find(d =>
            d.packageName === key || key.startsWith(d.packageName + '/')
        );
        if (dep) {
            return this.validateDependencyKey(key, dep);
        }

        if (key.startsWith('./') || key.startsWith('../')) {
            return this.validateRelativeKey(key);
        }

        return this.validateSourceKey(key, sourceBasePath);
    }

    private validateSystemApi(key: string): string {
        if (!this.systemApiPathSection[key]) {
            throw new DriverError(
                LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_INCORRECT_MOCK_CONFIG,
                    'Incorrect mock configuration.',
                    `System API "${key}" does not exist`
                )
            );
        }
        return key;
    }

    private validateDependencyKey(key: string, dep: DependencyModuleConfig): string {
        if (dep.packageName === key) {
            return key;
        }
        const relativePath = key.substring(dep.packageName.length + 1);
        const keyFullPath = this.resolveAndVerifyPath(dep.modulePath, relativePath);
        if (!keyFullPath) {
            throw new DriverError(
                LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_INCORRECT_MOCK_CONFIG,
                    'Incorrect mock configuration.',
                    `The key file for "${key}" is invalid. ` +
                    `File not found under ${dep.modulePath}.`
                )
            );
        }
        this.ensureUseStatic(key, keyFullPath);
        return keyFullPath;
    }

    private validateRelativeKey(key: string): string {
        const mockConfigDir = path.dirname(this.mockParams.mockConfigPath);
        const keyFullPath = this.resolveAndVerifyPath(mockConfigDir, key);
        if (!keyFullPath) {
            throw new DriverError(
                LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_INCORRECT_MOCK_CONFIG,
                    'Incorrect mock configuration.',
                    `The key file for "${key}" is invalid. ` +
                    `File not found.`
                )
            );
        }
        this.ensureUseStatic(key, keyFullPath);
        return keyFullPath;
    }

    private validateSourceKey(key: string, sourceBasePath: string): string {
        const sourceEtsPath = path.resolve(sourceBasePath, 'src', 'main', 'ets');
        const keyFullPath = this.resolveAndVerifyPath(sourceEtsPath, key);
        if (!keyFullPath) {
            throw new DriverError(
                LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_INCORRECT_MOCK_CONFIG,
                    'Incorrect mock configuration.',
                    `The key file for "${key}" is invalid. ` +
                    `Key must be a system API (starting with @ohos, @arkts, @kit), ` +
                    `a dependency package name, ` +
                    `or a source file.`
                )
            );
        }
        this.ensureUseStatic(key, keyFullPath);
        return keyFullPath;
    }

    private ensureUseStatic(key: string, filePath: string): void {
        if (!isFirstLineUseStatic(filePath)) {
            throw new DriverError(
                LogDataFactory.newInstance(
                    ErrorCode.BUILDSYSTEM_INCORRECT_MOCK_CONFIG,
                    'Incorrect mock configuration.',
                    `The key file for "${key}" is invalid. ` +
                    `The first line of the file must be 'use static'. ` +
                    `File path: ${filePath}`
                )
            );
        }
    }

    public getMockConfigInfo(): MockConfigInfo {
        return this.mockConfigInfo;
    }

    private resolveAndVerifyPath(basePath: string, partialPath: string): string | null {
        let fullPath = path.resolve(basePath, partialPath);
        let pathExists = fs.existsSync(fullPath);
        if (!pathExists && !path.extname(fullPath)) {
            fullPath += ETS_SUFFIX;
            pathExists = fs.existsSync(fullPath);
        }
        return pathExists ? fullPath : null;
    }
}
