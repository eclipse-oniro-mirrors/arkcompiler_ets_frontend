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

import { ArkTSConfig, ArkTSConfigGenerator } from '../../../src/build/generate_arktsconfig';
import { DependencyItem, ModuleInfo } from '../../../src/types';

/**
 * Mock ArkTSConfigGenerator for testing mergeArktsConfigByDependencies
 * We need to mock the getInstance static method to return our mock instance
 */
const mockConfigs = new Map<string, any>();

// Create a mock generator instance that can be returned by getInstance
const mockGeneratorInstance = {
    configs: mockConfigs,
    getArktsConfigByPackageName: function(packageName: string): any {
        return mockConfigs.get(packageName);
    },
    registerConfig: function(packageName: string, config: any): void {
        mockConfigs.set(packageName, config);
    },
    clear: function(): void {
        mockConfigs.clear();
    }
} as any;

// Mock the getInstance static method
jest.spyOn(ArkTSConfigGenerator, 'getInstance').mockReturnValue(mockGeneratorInstance);

/**
 * Helper function to create mock ModuleInfo
 */
function createMockModuleInfo(packageName: string): ModuleInfo {
    return {
        isMainModule: false,
        packageName: packageName,
        moduleRootPath: `/test/${packageName}`,
        moduleType: 'feature',
        sourceRoots: [`/test/${packageName}`],
        entryFile: `/test/${packageName}/index.ts`,
        arktsConfigFile: `/test/${packageName}/arktsConfig.json`,
        compileFileInfos: [],
        dynamicDepModuleInfos: new Map<string, ModuleInfo>(),
        staticDepModuleInfos: new Map<string, ModuleInfo>(),
        dependenciesSet: new Set(),
        dependentSet: new Set(),
        declgenV1OutPath: undefined,
        declgenV2OutPath: undefined,
        declgenBridgeCodePath: undefined,
        byteCodeHar: false,
    } as any;
}

/**
 * Helper function to create mock ArkTSConfig with test data
 */
function createMockArkTSConfig(packageName: string, paths: Record<string, string[]> = {}, deps: Record<string, DependencyItem> = {}): ArkTSConfig {
    const moduleInfo = createMockModuleInfo(packageName);
    const config = new ArkTSConfig(moduleInfo, '/cache', '/project', '/test/loader_out/default/etsFortgz');
    config.addPathMappings(paths);
    config.addDependencies(deps);
    return config;
}

/**
 * Test Suite for mergeArktsConfigByDependencies
 * Tests include:
 * - Normal dependency merging
 * - Circular dependency issues (exposing stack overflow problems)
 * - Multiple independent dependencies
 * - Deep dependency trees
 */
describe('ArkTSConfig - mergeArktsConfigByDependencies', () => {
    let mainConfig: ArkTSConfig;

    beforeEach(() => {
        // Clear the mock configs before each test
        mockConfigs.clear();
        mainConfig = createMockArkTSConfig('entry');
    });

    describe('Basic Dependency Merging', () => {
        test('should merge configurations of single dependency', () => {
            const depConfig = createMockArkTSConfig('dep1', {
                '@test/dep1': ['/path/to/dep1']
            });

            mockGeneratorInstance.registerConfig('dep1', depConfig);

            const dependencies: Set<string> = new Set(['dep1']);
            const dependenciesSets: Map<string, Set<string>> = new Map([
                ['dep1', new Set()],
                ['entry', new Set(['dep1'])]
            ]);

            mainConfig.mergeArktsConfigByDependencies(dependencies, dependenciesSets);

            expect(mainConfig.pathSection['@test/dep1']).toEqual(['/path/to/dep1']);
        });

        test('should merge configurations of multiple independent dependencies', () => {
            const dep1 = createMockArkTSConfig('dep1', {
                '@test/dep1': ['/path/to/dep1']
            });
            const dep2 = createMockArkTSConfig('dep2', {
                '@test/dep2': ['/path/to/dep2']
            });
            const dep3 = createMockArkTSConfig('dep3', {
                '@test/dep3': ['/path/to/dep3']
            });

            mockGeneratorInstance.registerConfig('dep1', dep1);
            mockGeneratorInstance.registerConfig('dep2', dep2);
            mockGeneratorInstance.registerConfig('dep3', dep3);

            const dependencies: Set<string> = new Set(['dep1', 'dep2', 'dep3']);
            const dependenciesSets: Map<string, Set<string>> = new Map([
                ['dep1', new Set()],
                ['dep2', new Set()],
                ['dep3', new Set()],
                ['entry', new Set(['dep1', 'dep2', 'dep3'])]
            ]);

            mainConfig.mergeArktsConfigByDependencies(dependencies, dependenciesSets);

            expect(mainConfig.pathSection['@test/dep1']).toEqual(['/path/to/dep1']);
            expect(mainConfig.pathSection['@test/dep2']).toEqual(['/path/to/dep2']);
            expect(mainConfig.pathSection['@test/dep3']).toEqual(['/path/to/dep3']);
        });

        test('should merge dependencies from dependency modules', () => {
            const depConfig = createMockArkTSConfig('dep1');
            depConfig.addDependencies({
                '@ohos.system': { language: 'ets', path: '/system.abc', ohmUrl: '@ohos.system' } as DependencyItem
            });

            mockGeneratorInstance.registerConfig('dep1', depConfig);

            const dependencies: Set<string> = new Set(['dep1']);
            const dependenciesSets: Map<string, Set<string>> = new Map([
                ['dep1', new Set()],
                ['entry', new Set(['dep1'])]
            ]);

            mainConfig.mergeArktsConfigByDependencies(dependencies, dependenciesSets);

            expect(mainConfig.dependencies['@ohos.system']).toBeDefined();
        });
    });

    describe('Deep Dependency Trees', () => {
        test('should merge configurations in deep dependency tree', () => {
            const dep3 = createMockArkTSConfig('dep3', {
                '@test/dep3': ['/path/to/dep3']
            });
            const dep2 = createMockArkTSConfig('dep2', {
                '@test/dep2': ['/path/to/dep2']
            });
            const dep1 = createMockArkTSConfig('dep1', {
                '@test/dep1': ['/path/to/dep1']
            });

            mockGeneratorInstance.registerConfig('dep1', dep1);
            mockGeneratorInstance.registerConfig('dep2', dep2);
            mockGeneratorInstance.registerConfig('dep3', dep3);

            const dependencies: Set<string> = new Set(['dep1']);
            const dependenciesSets: Map<string, Set<string>> = new Map([
                ['entry', new Set(['dep1'])],
                ['dep1', new Set(['dep2'])],
                ['dep2', new Set(['dep3'])],
                ['dep3', new Set()]
            ]);

            mainConfig.mergeArktsConfigByDependencies(dependencies, dependenciesSets);

            expect(mainConfig.pathSection['@test/dep1']).toEqual(['/path/to/dep1']);
            expect(mainConfig.pathSection['@test/dep2']).toEqual(['/path/to/dep2']);
            expect(mainConfig.pathSection['@test/dep3']).toEqual(['/path/to/dep3']);
        });

        test('should handle multiple branches in dependency tree', () => {
            const depA = createMockArkTSConfig('depA', { '@test/depA': ['/path/A'] });
            const depB = createMockArkTSConfig('depB', { '@test/depB': ['/path/B'] });
            const depC = createMockArkTSConfig('depC', { '@test/depC': ['/path/C'] });
            const depD = createMockArkTSConfig('depD', { '@test/depD': ['/path/D'] });

            mockGeneratorInstance.registerConfig('depA', depA);
            mockGeneratorInstance.registerConfig('depB', depB);
            mockGeneratorInstance.registerConfig('depC', depC);
            mockGeneratorInstance.registerConfig('depD', depD);

            const dependencies: Set<string> = new Set(['depA']);
            const dependenciesSets: Map<string, Set<string>> = new Map([
                ['entry', new Set(['depA'])],
                ['depA', new Set(['depB', 'depC'])],
                ['depB', new Set()],
                ['depC', new Set(['depD'])],
                ['depD', new Set()]
            ]);

            mainConfig.mergeArktsConfigByDependencies(dependencies, dependenciesSets);

            expect(mainConfig.pathSection['@test/depA']).toEqual(['/path/A']);
            expect(mainConfig.pathSection['@test/depB']).toEqual(['/path/B']);
            expect(mainConfig.pathSection['@test/depC']).toEqual(['/path/C']);
            expect(mainConfig.pathSection['@test/depD']).toEqual(['/path/D']);
        });
    });

    describe('Circular Dependency Handling', () => {
        test('should handle simple circular dependency (A -> B -> A) gracefully', () => {
            const depA = createMockArkTSConfig('depA', { '@test/depA': ['/path/A'] });
            const depB = createMockArkTSConfig('depB', { '@test/depB': ['/path/B'] });

            mockGeneratorInstance.registerConfig('depA', depA);
            mockGeneratorInstance.registerConfig('depB', depB);

            const dependencies: Set<string> = new Set(['depA']);
            const dependenciesSets: Map<string, Set<string>> = new Map([
                ['entry', new Set(['depA'])],
                ['depA', new Set(['depB'])],
                ['depB', new Set(['depA'])]
            ]);

            // Should handle circular dependency without infinite recursion
            // TODO: Implement circular dependency detection in mergeArktsConfigByDependencies
            mainConfig.mergeArktsConfigByDependencies(dependencies, dependenciesSets);

            // Should merge both dependencies correctly
            expect(mainConfig.pathSection['@test/depA']).toEqual(['/path/A']);
            expect(mainConfig.pathSection['@test/depB']).toEqual(['/path/B']);
        });

        test('should handle complex circular dependency (A -> B -> C -> D -> B) gracefully', () => {
            const depA = createMockArkTSConfig('depA', { '@test/depA': ['/path/A'] });
            const depB = createMockArkTSConfig('depB', { '@test/depB': ['/path/B'] });
            const depC = createMockArkTSConfig('depC', { '@test/depC': ['/path/C'] });
            const depD = createMockArkTSConfig('depD', { '@test/depD': ['/path/D'] });

            mockGeneratorInstance.registerConfig('depA', depA);
            mockGeneratorInstance.registerConfig('depB', depB);
            mockGeneratorInstance.registerConfig('depC', depC);
            mockGeneratorInstance.registerConfig('depD', depD);

            const dependencies: Set<string> = new Set(['depA']);
            const dependenciesSets: Map<string, Set<string>> = new Map([
                ['entry', new Set(['depA'])],
                ['depA', new Set(['depB'])],
                ['depB', new Set(['depC'])],
                ['depC', new Set(['depD'])],
                ['depD', new Set(['depB'])]
            ]);

            // Should handle circular dependency without infinite recursion
            // TODO: Implement circular dependency detection in mergeArktsConfigByDependencies
            mainConfig.mergeArktsConfigByDependencies(dependencies, dependenciesSets);

            // Should merge all dependencies correctly
            expect(mainConfig.pathSection['@test/depA']).toEqual(['/path/A']);
            expect(mainConfig.pathSection['@test/depB']).toEqual(['/path/B']);
            expect(mainConfig.pathSection['@test/depC']).toEqual(['/path/C']);
            expect(mainConfig.pathSection['@test/depD']).toEqual(['/path/D']);
        });

        test('should handle self-referencing dependency (A -> A) gracefully', () => {
            const depA = createMockArkTSConfig('depA', { '@test/depA': ['/path/A'] });

            mockGeneratorInstance.registerConfig('depA', depA);

            const dependencies: Set<string> = new Set(['depA']);
            const dependenciesSets: Map<string, Set<string>> = new Map([
                ['entry', new Set(['depA'])],
                ['depA', new Set(['depA'])]
            ]);

            // Should handle self-referencing dependency without infinite recursion
            // TODO: Implement circular dependency detection in mergeArktsConfigByDependencies
            mainConfig.mergeArktsConfigByDependencies(dependencies, dependenciesSets);

            // Should merge the dependency correctly
            expect(mainConfig.pathSection['@test/depA']).toEqual(['/path/A']);
        });

        test('should handle multiple independent circular dependencies gracefully', () => {
            const depA = createMockArkTSConfig('depA', { '@test/depA': ['/path/A'] });
            const depB = createMockArkTSConfig('depB', { '@test/depB': ['/path/B'] });
            const depC = createMockArkTSConfig('depC', { '@test/depC': ['/path/C'] });
            const depD = createMockArkTSConfig('depD', { '@test/depD': ['/path/D'] });

            mockGeneratorInstance.registerConfig('depA', depA);
            mockGeneratorInstance.registerConfig('depB', depB);
            mockGeneratorInstance.registerConfig('depC', depC);
            mockGeneratorInstance.registerConfig('depD', depD);

            const dependencies: Set<string> = new Set(['depA', 'depC']);
            const dependenciesSets: Map<string, Set<string>> = new Map([
                ['entry', new Set(['depA', 'depC'])],
                ['depA', new Set(['depB'])],
                ['depB', new Set(['depA'])],
                ['depC', new Set(['depD'])],
                ['depD', new Set(['depC'])]
            ]);

            // Should handle multiple circular dependencies without infinite recursion
            // TODO: Implement circular dependency detection in mergeArktsConfigByDependencies
            mainConfig.mergeArktsConfigByDependencies(dependencies, dependenciesSets);

            // Should merge all dependencies correctly
            expect(mainConfig.pathSection['@test/depA']).toEqual(['/path/A']);
            expect(mainConfig.pathSection['@test/depB']).toEqual(['/path/B']);
            expect(mainConfig.pathSection['@test/depC']).toEqual(['/path/C']);
            expect(mainConfig.pathSection['@test/depD']).toEqual(['/path/D']);
        });
    });

    describe('Edge Cases', () => {
        test('should handle empty dependencies set', () => {
            const dependencies: Set<string> = new Set();
            const dependenciesSets: Map<string, Set<string>> = new Map([]);

            expect(() => {
                mainConfig.mergeArktsConfigByDependencies(dependencies, dependenciesSets);
            }).not.toThrow();

            expect(Object.keys(mainConfig.pathSection).length).toBe(0);
        });

        test('should handle dependencies not registered in generator', () => {
            const dependencies: Set<string> = new Set(['nonexistent']);
            const dependenciesSets: Map<string, Set<string>> = new Map([
                ['entry', new Set(['nonexistent'])],
                ['nonexistent', new Set()]
            ]);

            // This will throw when trying to get the config
            expect(() => {
                mainConfig.mergeArktsConfigByDependencies(dependencies, dependenciesSets);
            }).toThrow();
        });

        test('should handle dependency with no sub-dependencies', () => {
            const dep1 = createMockArkTSConfig('dep1', { '@test/dep1': ['/path/to/dep1'] });

            mockGeneratorInstance.registerConfig('dep1', dep1);

            const dependencies: Set<string> = new Set(['dep1']);
            const dependenciesSets: Map<string, Set<string>> = new Map([
                ['entry', new Set(['dep1'])],
                ['dep1', new Set()]
            ]);

            mainConfig.mergeArktsConfigByDependencies(dependencies, dependenciesSets);

            expect(mainConfig.pathSection['@test/dep1']).toEqual(['/path/to/dep1']);
        });

        test('should preserve existing data in target config', () => {
            mainConfig.addPathMappings({ '@entry/own': ['/entry/path'] });

            const dep1 = createMockArkTSConfig('dep1', { '@test/dep1': ['/path/to/dep1'] });

            mockGeneratorInstance.registerConfig('dep1', dep1);

            const dependencies: Set<string> = new Set(['dep1']);
            const dependenciesSets: Map<string, Set<string>> = new Map([
                ['entry', new Set(['dep1'])],
                ['dep1', new Set()]
            ]);

            mainConfig.mergeArktsConfigByDependencies(dependencies, dependenciesSets);

            expect(mainConfig.pathSection['@entry/own']).toEqual(['/entry/path']);
            expect(mainConfig.pathSection['@test/dep1']).toEqual(['/path/to/dep1']);
        });
    });

    describe('Path and Deduplication Merging', () => {
        test('should deduplicate paths when dependencies have same aliases', () => {
            const dep1 = createMockArkTSConfig('dep1', { '@common': ['/path1'] });
            const dep2 = createMockArkTSConfig('dep2', { '@common': ['/path2'] });

            mockGeneratorInstance.registerConfig('dep1', dep1);
            mockGeneratorInstance.registerConfig('dep2', dep2);

            const dependencies: Set<string> = new Set(['dep1', 'dep2']);
            const dependenciesSets: Map<string, Set<string>> = new Map([
                ['entry', new Set(['dep1', 'dep2'])],
                ['dep1', new Set()],
                ['dep2', new Set()]
            ]);

            mainConfig.mergeArktsConfigByDependencies(dependencies, dependenciesSets);

            expect(mainConfig.pathSection['@common']).toEqual(['/path1', '/path2']);
        });

        test('should merge dependencies with overlapping data', () => {
            const dep1 = createMockArkTSConfig('dep1');
            dep1.addDependencies({
                '@common': { language: 'js', path: '/common/path1', ohmUrl: '@common' } as DependencyItem
            });

            const dep2 = createMockArkTSConfig('dep2');
            dep2.addDependencies({
                '@common': { language: 'js', path: '/common/path2', ohmUrl: '@common' } as DependencyItem
            });

            mockGeneratorInstance.registerConfig('dep1', dep1);
            mockGeneratorInstance.registerConfig('dep2', dep2);

            const dependencies: Set<string> = new Set(['dep1', 'dep2']);
            const dependenciesSets: Map<string, Set<string>> = new Map([
                ['entry', new Set(['dep1', 'dep2'])],
                ['dep1', new Set()],
                ['dep2', new Set()]
            ]);

            mainConfig.mergeArktsConfigByDependencies(dependencies, dependenciesSets);

            expect(mainConfig.dependencies['@common']).toBeDefined();
        });
    });
});
