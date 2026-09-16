/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import * as path from 'path';
import * as fs from 'fs';
import { BaseMode } from '../../../src/build/base_mode';
import { ErrorCode, DriverError } from '../../../src/util/error';

jest.mock('fs');
jest.mock('node:fs', () => jest.requireMock('fs'));
jest.mock('../../../src/logger', () => {
    const actual = jest.requireActual('../../../src/logger');
    const loggerInstance = {
        printDebug: jest.fn(),
        printInfo: jest.fn(),
        printWarn: jest.fn(),
        printError: jest.fn(),
        printErrorAndExit: jest.fn()
    };
    return {
        Logger: {
            getInstance: jest.fn(() => loggerInstance)
        },
        LogDataFactory: actual.LogDataFactory,
        LogData: actual.LogData
    };
});

jest.mock('../../../src/init/init_koala_modules', () => ({
    initKoalaModules: jest.fn(() => ({
        arkts: {},
        arktsGlobal: {}
    }))
}));

jest.mock('../../../src/util/utils', () => ({
    ...jest.requireActual('../../../src/util/utils'),
    ensurePathExists: jest.fn(),
    safeRealpath: jest.fn((p: string) => p)
}));

const mockEts = {
    initalize: jest.fn(),
    compile: jest.fn(),
    declgenV1: jest.fn(),
    finalize: jest.fn()
};
jest.mock('../../../src/util/ets2panda', () => ({
    Ets2panda: {
        getInstance: jest.fn(() => mockEts),
        destroyInstance: jest.fn()
    }
}));

const DYNAMIC_HAR_INDEX = { language: 'js', path: '/proj/dynamic_har/index.d.ets', sourceFilePath: '/proj/dynamic_har/index.ets', ohmUrl: 'dyn&bundle&mod&1.0.0' };
const DYNAMIC_HAR_SUB = { language: 'js', path: '/proj/dynamic_har/sub.d.ets', sourceFilePath: '/proj/dynamic_har/sub.ets', ohmUrl: 'dynsub&bundle&mod&1.0.0' };
const SDK_HILOG = { language: 'js', path: '/sdk/@ohos.hilog.d.ets', ohmUrl: '@ohos:hilog' };

function createOhmUrlMapCtx(): any {
    const ctx: any = Object.create((BaseMode as any).prototype);
    ctx.moduleInfos = new Map([
        ['entry', { packageName: 'entry', moduleName: 'entryname', moduleRootPath: '/proj/entry', dependencies: ['dynamic_har'] }],
        ['dynamic_har', { packageName: 'dynamic_har', moduleRootPath: '/proj/dynamic_har', dependencies: [], language: '1.1' }]
    ]);
    ctx.buildConfig = {
        cachePath: '/mock/cache',
        bundleName: 'bundle',
        packageName: 'entry',
        moduleType: 'entry',
        enableDeclgenEts2Ts: false
    };
    ctx.interopConfig = { buildOption: { stripInteropMapping: false }, targets: new Map() };
    return ctx;
}

function getWrittenMapContent(namePart: string): string {
    const call = (fs.writeFileSync as jest.Mock).mock.calls.find((c: unknown[]) =>
        typeof c[0] === 'string' && (c[0] as string).includes(namePart));
    expect(call).toBeDefined();
    return call![1] as string;
}

describe('BaseMode interop-config', () => {
    beforeEach(() => {
        jest.clearAllMocks();
        (fs.existsSync as jest.Mock).mockReturnValue(false);
        (fs.readFileSync as jest.Mock).mockReturnValue('{}');
        (fs.writeFileSync as jest.Mock).mockReturnValue(undefined);
        (fs.statSync as jest.Mock).mockReturnValue({ isFile: () => true });
    });

    test('generateDependencyOhmUrlMap generates the full map when stripInteropMapping is disabled', () => {
        const fn = (BaseMode as any).prototype.generateDependencyOhmUrlMap;
        const arktsConfig = {
            dependencies: {
                'dynamic_har/index': DYNAMIC_HAR_INDEX,
                'dynamic_har/sub': DYNAMIC_HAR_SUB,
                'dynamic/@ohos.hilog': SDK_HILOG
            }
        };
        const ctx = createOhmUrlMapCtx();

        fn.call(ctx, arktsConfig);

        const bundleMap = getWrittenMapContent('bundle_dependency_ohmurl_map.ets');
        expect(bundleMap).toContain('map_abc.set(');
        expect(bundleMap).toContain('map_ohmurl.set("dynamic_har/index", "dyn&bundle&mod&1.0.0");');
        expect(bundleMap).toContain('map_ohmurl.set("dynamic_har/sub", "dynsub&bundle&mod&1.0.0");');
        expect(bundleMap).not.toContain('@ohos.hilog');

        const dynamicMap = getWrittenMapContent('/_dependency_ohmurl_map.ets');
        expect(dynamicMap).toContain('map_ohmurl.set("dynamic_har/index", "dyn&bundle&mod&1.0.0");');
        expect(dynamicMap).toContain('map_ohmurl.set("@ohos.hilog", "@ohos:hilog");');
    });

    test('generateDependencyOhmUrlMap maps only configured dynamic entries when interop-config.json5 exists', () => {
        const fn = (BaseMode as any).prototype.generateDependencyOhmUrlMap;
        const arktsConfig = {
            dependencies: {
                'dynamic_har/index': DYNAMIC_HAR_INDEX,
                'dynamic_har/sub': DYNAMIC_HAR_SUB,
                'dynamic/@ohos.hilog': SDK_HILOG
            }
        };
        const ctx = createOhmUrlMapCtx();
        ctx.interopConfig = {
            buildOption: { stripInteropMapping: true },
            targets: new Map([
                ['dynamic_har', {
                    kind: 'items',
                    staticFiles: [],
                    dynamicFiles: [{
                        packageName: 'dynamic_har',
                        relativePath: 'index.ets',
                        filePath: '/proj/dynamic_har/index.ets'
                    }]
                }]
            ])
        };

        fn.call(ctx, arktsConfig);

        const bundleMap = getWrittenMapContent('bundle_dependency_ohmurl_map.ets');
        expect(bundleMap).toContain('map_abc.set(');
        expect(bundleMap).toContain('map_ohmurl.set("dynamic_har/index", "dyn&bundle&mod&1.0.0");');
        expect(bundleMap).not.toContain('dynamic_har/sub');
        expect(bundleMap).not.toContain('@ohos.hilog');

        const dynamicMap = getWrittenMapContent('/_dependency_ohmurl_map.ets');
        expect(dynamicMap).toContain('map_ohmurl.set("dynamic_har/index", "dyn&bundle&mod&1.0.0");');
        expect(dynamicMap).not.toContain('dynamic_har/sub');
        expect(dynamicMap).toContain('map_ohmurl.set("@ohos.hilog", "@ohos:hilog");');
    });

    test('generateDependencyOhmUrlMap maps all files of a package target from interop-config.json5', () => {
        const fn = (BaseMode as any).prototype.generateDependencyOhmUrlMap;
        const arktsConfig = {
            dependencies: {
                'dynamic_har/index': DYNAMIC_HAR_INDEX,
                'dynamic_har/sub': DYNAMIC_HAR_SUB,
                'dynamic/@ohos.hilog': SDK_HILOG
            }
        };
        const ctx = createOhmUrlMapCtx();
        ctx.interopConfig = {
            buildOption: { stripInteropMapping: true },
            targets: new Map([
                ['dynamic_har', {
                    kind: 'package',
                    moduleInfo: { packageName: 'dynamic_har', modulePath: '/proj/dynamic_har', dependencies: [] }
                }]
            ])
        };

        fn.call(ctx, arktsConfig);

        const bundleMap = getWrittenMapContent('bundle_dependency_ohmurl_map.ets');
        expect(bundleMap).toContain('map_ohmurl.set("dynamic_har/index", "dyn&bundle&mod&1.0.0");');
        expect(bundleMap).toContain('map_ohmurl.set("dynamic_har/sub", "dynsub&bundle&mod&1.0.0");');
        expect(bundleMap).not.toContain('@ohos.hilog');

        const dynamicMap = getWrittenMapContent('/_dependency_ohmurl_map.ets');
        expect(dynamicMap).toContain('map_ohmurl.set("@ohos.hilog", "@ohos:hilog");');
    });

    test('generateDependencyOhmUrlMap strips all non-SDK entries when stripInteropMapping is set without module configs', () => {
        const fn = (BaseMode as any).prototype.generateDependencyOhmUrlMap;
        const arktsConfig = {
            dependencies: {
                'dynamic_har/index': DYNAMIC_HAR_INDEX,
                'dynamic_har/sub': DYNAMIC_HAR_SUB,
                'dynamic/@ohos.hilog': SDK_HILOG
            }
        };
        const ctx = createOhmUrlMapCtx();
        ctx.interopConfig = { buildOption: { stripInteropMapping: true }, targets: new Map() };

        fn.call(ctx, arktsConfig);

        const bundleMap = getWrittenMapContent('bundle_dependency_ohmurl_map.ets');
        expect(bundleMap).toContain('map_abc.set(');
        expect(bundleMap).toContain('let map_ohmurl = new Map<string, string>();');
        expect(bundleMap).not.toContain('map_ohmurl.set("dynamic_har');
        expect(bundleMap).not.toContain('@ohos.hilog');

        // dynamic/ SDK entries are never stripped
        const dynamicMap = getWrittenMapContent('/_dependency_ohmurl_map.ets');
        expect(dynamicMap).toContain('map_ohmurl.set("@ohos.hilog", "@ohos:hilog");');
        expect(dynamicMap).not.toContain('map_ohmurl.set("dynamic_har');
    });

    test('resolveInteropConfigs defaults the project-level build option when no interopConfigPath is passed', () => {
        const fn = (BaseMode as any).prototype.resolveInteropConfigs;
        const ctx = createInteropResolveCtx(undefined);

        fn.call(ctx);

        expect(ctx.interopConfig).toEqual({ buildOption: { stripInteropMapping: false }, targets: new Map() });
        expect(fs.readFileSync).not.toHaveBeenCalled();
    });

    test('resolveInteropConfigs parses the project-level interopBuildOption', () => {
        const fn = (BaseMode as any).prototype.resolveInteropConfigs;
        (fs.readFileSync as jest.Mock).mockImplementation((p: string) => {
            if (p === '/proj/interop-config.json5') {
                return '{ interopBuildOption: { stripInteropMapping: true } }';
            }
            return '{}';
        });
        const ctx = createInteropResolveCtx(undefined);
        ctx.buildConfig.interopConfigPath = '/proj/interop-config.json5';

        fn.call(ctx);

        expect(ctx.interopConfig.buildOption).toEqual({ stripInteropMapping: true });
        expect(ctx.interopConfig.targets.size).toBe(0);
    });

    test('resolveInteropConfigs defaults stripInteropMapping when the project config omits interopBuildOption', () => {
        const fn = (BaseMode as any).prototype.resolveInteropConfigs;
        (fs.readFileSync as jest.Mock).mockImplementation((p: string) => {
            if (p === '/proj/interop-config.json5') {
                return '{ }';
            }
            return '{}';
        });
        const ctx = createInteropResolveCtx(undefined);
        ctx.buildConfig.interopConfigPath = '/proj/interop-config.json5';

        fn.call(ctx);

        expect(ctx.interopConfig.buildOption).toEqual({ stripInteropMapping: false });
    });

    test('resolveInteropConfigs converts invalid project interop-config.json5 to DriverError', () => {
        const fn = (BaseMode as any).prototype.resolveInteropConfigs;
        (fs.readFileSync as jest.Mock).mockImplementation((p: string) => {
            if (p === '/proj/interop-config.json5') {
                return '{ invalid json5 !!!';
            }
            return '{}';
        });
        const ctx = createInteropResolveCtx(undefined);
        ctx.buildConfig.interopConfigPath = '/proj/interop-config.json5';

        expect(() => fn.call(ctx)).toThrow(DriverError);
        try {
            fn.call(ctx);
        } catch (error) {
            expect((error as DriverError).logData.code).toBe(ErrorCode.BUILDSYSTEM_INTEROP_CONFIG_RESOLVE_FAIL);
            expect((error as DriverError).toString()).toContain('project interop configuration');
        }
    });

    test('resolveInteropConfigs resolves own and dependency dynamic entries', () => {
        const fn = (BaseMode as any).prototype.resolveInteropConfigs;
        (fs.readFileSync as jest.Mock).mockImplementation((p: string) => {
            if (p === '/proj/dynamic_har/interop-config.json5') {
                return '{ interopEntries: { dynamic: ["./index.ets"], ' +
                    'dependency: { source: { "static_har": { dynamic: ["helper.ts"] } } } } }';
            }
            return '{}';
        });
        const ctx = createInteropResolveCtx('/proj/dynamic_har/interop-config.json5');

        fn.call(ctx);

        expect(ctx.interopConfig.targets).toBeDefined();
        expect(ctx.interopConfig.targets.get('dynamic_har')).toEqual({
            kind: 'items',
            staticFiles: [],
            dynamicFiles: [{
                packageName: 'dynamic_har',
                relativePath: './index.ets',
                filePath: path.resolve('/proj/dynamic_har', './index.ets')
            }]
        });
        expect(ctx.interopConfig.targets.get('static_har')).toEqual({
            kind: 'items',
            staticFiles: [],
            dynamicFiles: [{
                packageName: 'static_har',
                relativePath: 'helper.ts',
                filePath: path.resolve('/proj/static_har', 'helper.ts')
            }]
        });
    });

    test('resolveInteropConfigs merges duplicate file entries by path', () => {
        const fn = (BaseMode as any).prototype.resolveInteropConfigs;
        (fs.readFileSync as jest.Mock).mockImplementation((p: string) => {
            if (p === '/proj/dynamic_har/interop-config.json5') {
                return '{ interopEntries: { dependency: { source: { "static_har": { dynamic: ["helper.ts"] } } } } }';
            }
            if (p === '/proj/static_har/interop-config.json5') {
                return '{ interopEntries: { dynamic: ["helper.ts", "extra.ts"] } }';
            }
            return '{}';
        });
        const ctx = createInteropResolveCtx('/proj/dynamic_har/interop-config.json5');
        ctx.moduleInfos.get('static_har').interopConfigPath = '/proj/static_har/interop-config.json5';

        fn.call(ctx);

        expect(ctx.interopConfig.targets.get('static_har')).toEqual({
            kind: 'items',
            staticFiles: [],
            dynamicFiles: [
                { packageName: 'static_har', relativePath: 'helper.ts', filePath: path.resolve('/proj/static_har', 'helper.ts') },
                { packageName: 'static_har', relativePath: 'extra.ts', filePath: path.resolve('/proj/static_har', 'extra.ts') }
            ]
        });
    });

    test('resolveInteropConfigs converts invalid interop-config.json5 to DriverError', () => {
        const fn = (BaseMode as any).prototype.resolveInteropConfigs;
        (fs.readFileSync as jest.Mock).mockImplementation((p: string) => {
            if (p === '/proj/dynamic_har/interop-config.json5') {
                return '{ invalid json5 !!!';
            }
            return '{}';
        });
        const ctx = createInteropResolveCtx('/proj/dynamic_har/interop-config.json5');

        expect(() => fn.call(ctx)).toThrow(DriverError);
        try {
            fn.call(ctx);
        } catch (error) {
            expect((error as DriverError).logData.code).toBe(ErrorCode.BUILDSYSTEM_INTEROP_CONFIG_RESOLVE_FAIL);
            expect((error as DriverError).toString()).toContain('interop configuration');
        }
    });
});

function createInteropResolveCtx(interopConfigPath?: string): any {
    const ctx: any = Object.create((BaseMode as any).prototype);
    ctx.buildConfig = {
        packageName: 'entry',
        moduleRootPath: '/proj/entry',
        projectRootPath: '/proj',
        cachePath: '/mock/cache',
        loaderOutPath: '/mock/output',
        interopConfigPath: undefined
    };
    ctx.moduleInfos = new Map([
        ['entry', {
            packageName: 'entry',
            moduleRootPath: '/proj/entry',
            dependencies: ['dynamic_har', 'static_har'],
            interopConfigPath: undefined
        }],
        ['dynamic_har', {
            packageName: 'dynamic_har',
            moduleRootPath: '/proj/dynamic_har',
            dependencies: [],
            interopConfigPath: interopConfigPath
        }],
        ['static_har', {
            packageName: 'static_har',
            moduleRootPath: '/proj/static_har',
            dependencies: []
        }]
    ]);
    ctx.interopConfig = { buildOption: { stripInteropMapping: false }, targets: new Map() };
    return ctx;
}
