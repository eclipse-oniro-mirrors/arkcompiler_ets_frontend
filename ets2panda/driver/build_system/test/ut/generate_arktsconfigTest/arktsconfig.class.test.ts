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

import { ArkTSConfig } from '../../../src/build/generate_arktsconfig';
import { DependencyItem, ModuleInfo } from '../../../src/types';

function createMockModuleInfo(overrides: Partial<ModuleInfo> = {}): ModuleInfo {
    return {
        packageName: 'testModule',
        moduleRootPath: '/test/module',
        sourceRoots: ['src'],
        arktsConfigFile: '/test/arktsconfig.json',
        compileFileInfos: [],
        dynamicDependencyModules: [],
        ...overrides
    } as any;
}

/**
 * arktsconfig management
 */
describe('ArkTSConfig - Constructor and Initialization', () => {
    let arktsConfig: ArkTSConfig;
    let moduleInfo: ModuleInfo;

    beforeEach(() => {
        moduleInfo = createMockModuleInfo();
        arktsConfig = new ArkTSConfig(moduleInfo, '/cache', '/project');
    });

    test('should initialize object structure correctly', () => {
        expect(arktsConfig.object).toBeDefined();
        expect(arktsConfig.object.compilerOptions).toBeDefined();
    });

    test('should set package name from moduleInfo', () => {
        expect(arktsConfig.packageName).toBe('testModule');
        expect(arktsConfig.compilerOptions.package).toBe('testModule');
    });

    test('should construct baseUrl from moduleRoot and sourceRoots', () => {
        const expectedBaseUrl = '/test/module/src';
        expect(arktsConfig.compilerOptions.baseUrl).toBe(expectedBaseUrl);
    });

    test('should set cacheDir correctly', () => {
        expect(arktsConfig.compilerOptions.cacheDir).toBe('/cache');
    });

    test('should set projectRootPath correctly', () => {
        expect(arktsConfig.compilerOptions.projectRootPath).toBe('/project');
    });

    test('should initialize empty paths object', () => {
        expect(arktsConfig.pathSection).toEqual({});
        expect(Object.keys(arktsConfig.pathSection).length).toBe(0);
    });

    test('should initialize empty dependencies object', () => {
        expect(arktsConfig.dependencies).toEqual({});
        expect(Object.keys(arktsConfig.dependencies).length).toBe(0);
    });

    test('should support custom package name', () => {
        const customInfo = createMockModuleInfo({ packageName: 'customPackage' });
        const customConfig = new ArkTSConfig(customInfo, '/cache', '/project');
        expect(customConfig.packageName).toBe('customPackage');
    });

    test('should support multiple source roots (uses first one)', () => {
        const multiSourceInfo = createMockModuleInfo({ sourceRoots: ['src', 'lib', 'util'] });
        const multiConfig = new ArkTSConfig(multiSourceInfo, '/cache', '/project');
        expect(multiConfig.compilerOptions.baseUrl).toBe('/test/module/src');
    });
});

/**
 * Path Mappings Management
 */
describe('ArkTSConfig - Path Mappings Management', () => {
    let arktsConfig: ArkTSConfig;

    beforeEach(() => {
        const moduleInfo = createMockModuleInfo();
        arktsConfig = new ArkTSConfig(moduleInfo, '/cache', '/project');
    });

    describe('Adding Single Path Mapping', () => {
        test('should add single path mapping', () => {
            arktsConfig.addPathMappings({ 'alias': ['/path/to/file'] });
            expect(arktsConfig.pathSection['alias']).toEqual(['/path/to/file']);
        });

        test('should add mapping with multiple path values', () => {
            arktsConfig.addPathMappings({ 'alias': ['/path1', '/path2', '/path3'] });
            expect(arktsConfig.pathSection['alias']).toEqual(['/path1', '/path2', '/path3']);
        });
    });

    describe('Batch Adding Path Mappings', () => {
        test('should batch add multiple path mappings', () => {
            arktsConfig.addPathMappings({
                'alias1': ['/path1'],
                'alias2': ['/path2'],
                'alias3': ['/path3']
            });
            expect(arktsConfig.pathSection['alias1']).toEqual(['/path1']);
            expect(arktsConfig.pathSection['alias2']).toEqual(['/path2']);
            expect(arktsConfig.pathSection['alias3']).toEqual(['/path3']);
        });

        test('should handle empty object correctly', () => {
            arktsConfig.addPathMappings({});
            expect(arktsConfig.pathSection).toEqual({});
        });
    });

    describe('Path Merging and Deduplication', () => {
        test('should merge paths for same key', () => {
            arktsConfig.addPathMappings({ 'alias': ['/path1'] });
            arktsConfig.addPathMappings({ 'alias': ['/path2'] });
            expect(arktsConfig.pathSection['alias']).toEqual(['/path1', '/path2']);
        });

        test('should deduplicate repeated paths', () => {
            arktsConfig.addPathMappings({ 'alias': ['/path1', '/path2'] });
            arktsConfig.addPathMappings({ 'alias': ['/path2', '/path3'] });
            expect(arktsConfig.pathSection['alias']).toEqual(['/path1', '/path2', '/path3']);
        });

        test('should maintain path order', () => {
            arktsConfig.addPathMappings({ 'alias': ['/a', '/b', '/c'] });
            arktsConfig.addPathMappings({ 'alias': ['/d', '/e'] });
            expect(arktsConfig.pathSection['alias'][0]).toBe('/a');
            expect(arktsConfig.pathSection['alias'][4]).toBe('/e');
        });

        test('should handle adding same path multiple times', () => {
            arktsConfig.addPathMappings({ 'alias': ['/path'] });
            arktsConfig.addPathMappings({ 'alias': ['/path'] });
            arktsConfig.addPathMappings({ 'alias': ['/path'] });
            expect(arktsConfig.pathSection['alias']).toEqual(['/path']);
        });
    });

    describe('Special Cases Handling', () => {
        test('should support special characters as keys', () => {
            arktsConfig.addPathMappings({
                '@scope/package': ['/path1'],
                'path.with.dots': ['/path2'],
                'path-with-dash': ['/path3']
            });
            expect(arktsConfig.pathSection['@scope/package']).toEqual(['/path1']);
            expect(arktsConfig.pathSection['path.with.dots']).toEqual(['/path2']);
            expect(arktsConfig.pathSection['path-with-dash']).toEqual(['/path3']);
        });

        test('should support absolute paths', () => {
            arktsConfig.addPathMappings({
                'alias': ['/absolute/path/to/file.d.ets']
            });
            expect(arktsConfig.pathSection['alias']).toEqual(['/absolute/path/to/file.d.ets']);
        });
    });
});

/**
 * Dependencies Management
 */
describe('ArkTSConfig - Dependencies Management', () => {
    let arktsConfig: ArkTSConfig;

    beforeEach(() => {
        const moduleInfo = createMockModuleInfo();
        arktsConfig = new ArkTSConfig(moduleInfo, '/cache', '/project');
    });

    describe('Adding Single Dependency', () => {
        test('should add JavaScript dependency', () => {
            const dep: DependencyItem = {
                language: 'js',
                path: '/mock/dep.d.ets',
                ohmUrl: 'testDep'
            };
            arktsConfig.addDependency({ name: 'testDep', item: dep });
            expect(arktsConfig.dependencies['testDep']).toEqual(dep);
        });

        test('should add ETS dependency', () => {
            const dep: DependencyItem = {
                language: 'ets',
                path: '/stdlib.abc',
                ohmUrl: 'std/core'
            };
            arktsConfig.addDependency({ name: 'std/core', item: dep });
            expect(arktsConfig.dependencies['std/core']).toEqual(dep);
        });

        test('should add dependency with alias', () => {
            const dep: DependencyItem = {
                language: 'js',
                path: '/path',
                ohmUrl: 'dep',
                alias: ['alias1', 'alias2']
            };
            arktsConfig.addDependency({ name: 'dep', item: dep });
            expect(arktsConfig.dependencies['dep'].alias).toEqual(['alias1', 'alias2']);
        });

        test('should add dependency with sourceFilePath', () => {
            const dep: DependencyItem = {
                language: 'js',
                path: '/decl.d.ets',
                sourceFilePath: '/source.ets',
                ohmUrl: 'dep'
            };
            arktsConfig.addDependency({ name: 'dep', item: dep });
            expect(arktsConfig.dependencies['dep'].sourceFilePath).toBe('/source.ets');
        });
    });

    describe('Dependency Merging', () => {
        test('should merge dependencies with same name using latest properties', () => {
            const dep1: DependencyItem = {
                language: 'js',
                path: '/path1',
                ohmUrl: 'dep',
                alias: ['a1']
            };
            const dep2: DependencyItem = {
                language: 'ets',
                path: '/path2',
                ohmUrl: 'dep2',
                alias: ['a2']
            };

            arktsConfig.addDependency({ name: 'dep', item: dep1 });
            arktsConfig.addDependency({ name: 'dep', item: dep2 });

            expect(arktsConfig.dependencies['dep'].language).toBe('ets');
            expect(arktsConfig.dependencies['dep'].path).toBe('/path2');
            expect(arktsConfig.dependencies['dep'].ohmUrl).toBe('dep2');
        });

        test('should merge and deduplicate aliases', () => {
            arktsConfig.addDependency({
                name: 'dep',
                item: { language: 'js', path: '/p', ohmUrl: 'dep', alias: ['a', 'b'] }
            });
            arktsConfig.addDependency({
                name: 'dep',
                item: { language: 'js', path: '/p', ohmUrl: 'dep', alias: ['b', 'c'] }
            });
            expect(arktsConfig.dependencies['dep'].alias).toEqual(['a', 'b', 'c']);
        });

        test('should maintain alias order', () => {
            arktsConfig.addDependency({
                name: 'dep',
                item: { language: 'js', path: '/p', ohmUrl: 'dep', alias: ['x', 'y'] }
            });
            arktsConfig.addDependency({
                name: 'dep',
                item: { language: 'js', path: '/p', ohmUrl: 'dep', alias: ['z', 'w'] }
            });
            const aliases = arktsConfig.dependencies['dep'].alias!;
            expect(aliases[0]).toBe('x');
            expect(aliases[1]).toBe('y');
            expect(aliases[2]).toBe('z');
            expect(aliases[3]).toBe('w');
        });
    });

    describe('Batch Adding Dependencies', () => {
        test('should batch add via addDependencies', () => {
            const deps = {
                'dep1': { language: 'js', path: '/p1', ohmUrl: 'd1' } as DependencyItem,
                'dep2': { language: 'js', path: '/p2', ohmUrl: 'd2' } as DependencyItem,
                'dep3': { language: 'ets', path: '/p3', ohmUrl: 'd3' } as DependencyItem
            };
            arktsConfig.addDependencies(deps);
            expect(arktsConfig.dependencies['dep1']).toEqual(deps['dep1']);
            expect(arktsConfig.dependencies['dep2']).toEqual(deps['dep2']);
            expect(arktsConfig.dependencies['dep3']).toEqual(deps['dep3']);
        });

        test('should trigger merge logic during batch add', () => {
            arktsConfig.addDependency({
                name: 'common',
                item: { language: 'js', path: '/p', ohmUrl: 'c', alias: ['a1'] }
            });

            const deps = {
                'common': { language: 'js', path: '/p', ohmUrl: 'c', alias: ['a2'] } as DependencyItem,
                'new': { language: 'js', path: '/n', ohmUrl: 'n' } as DependencyItem
            };
            arktsConfig.addDependencies(deps);

            expect(arktsConfig.dependencies['common'].alias).toEqual(['a1', 'a2']);
            expect(arktsConfig.dependencies['new']).toBeDefined();
        });

        test('should handle empty object correctly', () => {
            arktsConfig.addDependencies({});
            expect(arktsConfig.dependencies).toEqual({});
        });
    });

    describe('Special Cases Handling', () => {
        test('should handle dependencies without alias', () => {
            const dep: DependencyItem = {
                language: 'ets',
                path: '/stdlib.abc',
                ohmUrl: 'std/core'
            };
            arktsConfig.addDependency({ name: 'std/core', item: dep });
            expect(arktsConfig.dependencies['std/core'].alias).toBeUndefined();
        });

        test('should handle correctly when one has no alias during merge', () => {
            arktsConfig.addDependency({
                name: 'dep',
                item: { language: 'js', path: '/p', ohmUrl: 'd' }
            });
            arktsConfig.addDependency({
                name: 'dep',
                item: { language: 'js', path: '/p', ohmUrl: 'd', alias: ['a'] }
            });
            expect(arktsConfig.dependencies['dep'].alias).toEqual(['a']);
        });

        test('should support complex dependency names', () => {
            const complexNames = [
                '@ohos.app.ability',
                'std/core',
                'component/button',
                '@dynamic:@system.router'
            ];

            complexNames.forEach((name, index) => {
                arktsConfig.addDependency({
                    name,
                    item: { language: 'js', path: `/path${index}`, ohmUrl: name }
                });
            });

            complexNames.forEach(name => {
                expect(arktsConfig.dependencies[name]).toBeDefined();
            });
        });
    });
});

/**
 * arktsconfig merging
 */
describe('ArkTSConfig - Config Merging', () => {
    let arktsConfig: ArkTSConfig;
    let sourceConfig: ArkTSConfig;
    beforeEach(() => {
        const moduleInfo1 = createMockModuleInfo({ packageName: 'target' });
        const moduleInfo2 = createMockModuleInfo({ packageName: 'source' });
        arktsConfig = new ArkTSConfig(moduleInfo1, '/cache', '/project');
        sourceConfig = new ArkTSConfig(moduleInfo2, '/cache', '/project');
    });

    describe('Basic Merging Functionality', () => {
        test('should merge paths and dependencies', () => {
            sourceConfig.addPathMappings({ 'srcAlias': ['/src/path'] });
            sourceConfig.addDependencies({
                'srcDep': { language: 'js', path: '/src/dep', ohmUrl: 'srcDep' }
            });

            arktsConfig.mergeArktsConfig(sourceConfig);

            expect(arktsConfig.pathSection['srcAlias']).toEqual(['/src/path']);
            expect(arktsConfig.dependencies['srcDep']).toBeDefined();
        });

        test('should preserve target config original data', () => {
            arktsConfig.addPathMappings({ 'targetAlias': ['/target/path'] });
            arktsConfig.addDependencies({
                'targetDep': { language: 'js', path: '/target/dep', ohmUrl: 'targetDep' }
            });

            sourceConfig.addPathMappings({ 'srcAlias': ['/src/path'] });

            arktsConfig.mergeArktsConfig(sourceConfig);

            expect(arktsConfig.pathSection['targetAlias']).toEqual(['/target/path']);
            expect(arktsConfig.dependencies['targetDep']).toBeDefined();
            expect(arktsConfig.pathSection['srcAlias']).toEqual(['/src/path']);
        });

        test('should handle undefined source config correctly', () => {
            arktsConfig.addPathMappings({ 'alias': ['/path'] });
            const pathsBefore = { ...arktsConfig.pathSection };

            arktsConfig.mergeArktsConfig(undefined);

            expect(arktsConfig.pathSection).toEqual(pathsBefore);
        });
    });

    describe('Overlapping Data Merging', () => {
        test('should merge overlapping path mappings', () => {
            arktsConfig.addPathMappings({ 'common': ['/target/path'] });
            sourceConfig.addPathMappings({ 'common': ['/source/path'] });

            arktsConfig.mergeArktsConfig(sourceConfig);

            expect(arktsConfig.pathSection['common']).toEqual(['/target/path', '/source/path']);
        });

        test('should merge overlapping dependencies', () => {
            arktsConfig.addDependency({
                name: 'common',
                item: { language: 'js', path: '/c', ohmUrl: 'c', alias: ['a1'] }
            });
            sourceConfig.addDependency({
                name: 'common',
                item: { language: 'js', path: '/c', ohmUrl: 'c', alias: ['a2'] }
            });

            arktsConfig.mergeArktsConfig(sourceConfig);

            expect(arktsConfig.dependencies['common'].alias).toEqual(['a1', 'a2']);
        });

        test('should deduplicate merged data', () => {
            arktsConfig.addPathMappings({ 'alias': ['/path1', '/path2'] });
            sourceConfig.addPathMappings({ 'alias': ['/path2', '/path3'] });

            arktsConfig.mergeArktsConfig(sourceConfig);

            expect(arktsConfig.pathSection['alias']).toEqual(['/path1', '/path2', '/path3']);
        });
    });

    describe('Complex Merging Scenarios', () => {
        test('should support multiple merges', () => {
            const source1 = new ArkTSConfig(
                createMockModuleInfo({ packageName: 's1' }),
                '/cache',
                '/project'
            );
            const source2 = new ArkTSConfig(
                createMockModuleInfo({ packageName: 's2' }),
                '/cache',
                '/project'
            );

            source1.addPathMappings({ 'a1': ['/p1'] });
            source2.addPathMappings({ 'a2': ['/p2'] });

            arktsConfig.mergeArktsConfig(source1);
            arktsConfig.mergeArktsConfig(source2);

            expect(arktsConfig.pathSection['a1']).toEqual(['/p1']);
            expect(arktsConfig.pathSection['a2']).toEqual(['/p2']);
        });

        test('should support merging configs with large amounts of data', () => {
            for (let i = 0; i < 50; i++) {
                sourceConfig.addPathMappings({ [`alias${i}`]: [`/path${i}`] });
                sourceConfig.addDependencies({
                    [`dep${i}`]: { language: 'js', path: `/dep${i}`, ohmUrl: `dep${i}` }
                });
            }

            arktsConfig.mergeArktsConfig(sourceConfig);

            expect(Object.keys(arktsConfig.pathSection).length).toBe(50);
            expect(Object.keys(arktsConfig.dependencies).length).toBe(50);
        });

        test('should support chained merging', () => {
            const c1 = new ArkTSConfig(createMockModuleInfo(), '/cache', '/project');
            const c2 = new ArkTSConfig(createMockModuleInfo(), '/cache', '/project');
            const c3 = new ArkTSConfig(createMockModuleInfo(), '/cache', '/project');

            c1.addPathMappings({ 'a': ['/1'] });
            c2.addPathMappings({ 'b': ['/2'] });
            c3.addPathMappings({ 'c': ['/3'] });

            c2.mergeArktsConfig(c1);
            c3.mergeArktsConfig(c2);

            expect(c3.pathSection['a']).toEqual(['/1']);
            expect(c3.pathSection['b']).toEqual(['/2']);
            expect(c3.pathSection['c']).toEqual(['/3']);
        });
    });
});

/**
 * getters and setters
 */
describe('ArkTSConfig - Getters and Setters', () => {
    let arktsConfig: ArkTSConfig;

    beforeEach(() => {
        const moduleInfo = createMockModuleInfo({ packageName: 'testPkg' });
        arktsConfig = new ArkTSConfig(moduleInfo, '/cache', '/project');
    });

    describe('Getter Methods', () => {
        test('compilerOptions should return compiler options object', () => {
            const opts = arktsConfig.compilerOptions;
            expect(opts).toBeDefined();
            expect(opts.package).toBe('testPkg');
            expect(opts.baseUrl).toBeDefined();
            expect(opts.paths).toBeDefined();
            expect(opts.dependencies).toBeDefined();
        });

        test('packageName should return package name', () => {
            expect(arktsConfig.packageName).toBe('testPkg');
        });

        test('dependencies should return dependencies object', () => {
            const dep: DependencyItem = {
                language: 'js',
                path: '/test',
                ohmUrl: 'test'
            };
            arktsConfig.addDependency({ name: 'test', item: dep });

            const deps = arktsConfig.dependencies;
            expect(deps['test']).toEqual(dep);
        });

        test('pathSection should return path mappings object', () => {
            arktsConfig.addPathMappings({ 'alias': ['/path'] });

            const paths = arktsConfig.pathSection;
            expect(paths['alias']).toEqual(['/path']);
        });

        test('modifying getter returned object should affect internal state', () => {
            const paths = arktsConfig.pathSection;
            paths['newAlias'] = ['/new/path'];

            expect(arktsConfig.pathSection['newAlias']).toEqual(['/new/path']);
        });

        test('compilerOptions should contain all required fields', () => {
            const opts = arktsConfig.compilerOptions;
            expect(opts.package).toBeDefined();
            expect(opts.baseUrl).toBeDefined();
            expect(opts.paths).toBeDefined();
            expect(opts.dependencies).toBeDefined();
            expect(opts.cacheDir).toBeDefined();
            expect(opts.projectRootPath).toBeDefined();
        });
    });

    describe('Setter Methods', () => {
        test('useEmptyPackage should be settable to true', () => {
            arktsConfig.useEmptyPackage = true;
            expect(arktsConfig.object.compilerOptions.useEmptyPackage).toBe(true);
        });

        test('useEmptyPackage should be settable to false', () => {
            arktsConfig.useEmptyPackage = true;
            arktsConfig.useEmptyPackage = false;
            expect(arktsConfig.object.compilerOptions.useEmptyPackage).toBe(false);
        });

        test('useEmptyPackage initial value should be undefined', () => {
            expect(arktsConfig.object.compilerOptions.useEmptyPackage).toBeUndefined();
        });

        test('useEmptyPackage should be modifiable multiple times', () => {
            arktsConfig.useEmptyPackage = true;
            expect(arktsConfig.object.compilerOptions.useEmptyPackage).toBe(true);

            arktsConfig.useEmptyPackage = false;
            expect(arktsConfig.object.compilerOptions.useEmptyPackage).toBe(false);

            arktsConfig.useEmptyPackage = true;
            expect(arktsConfig.object.compilerOptions.useEmptyPackage).toBe(true);
        });
    });

    describe('Data Consistency', () => {
        test('accessing via different methods should return same data', () => {
            arktsConfig.addPathMappings({ 'test': ['/path'] });

            expect(arktsConfig.pathSection['test']).toEqual(['/path']);
            expect(arktsConfig.compilerOptions.paths['test']).toEqual(['/path']);
            expect(arktsConfig.object.compilerOptions.paths['test']).toEqual(['/path']);
        });

        test('modifying via different methods should stay consistent', () => {
            arktsConfig.pathSection['direct'] = ['/direct'];
            arktsConfig.addPathMappings({ 'method': ['/method'] });

            expect(arktsConfig.compilerOptions.paths['direct']).toEqual(['/direct']);
            expect(arktsConfig.compilerOptions.paths['method']).toEqual(['/method']);
        });
    });
});
