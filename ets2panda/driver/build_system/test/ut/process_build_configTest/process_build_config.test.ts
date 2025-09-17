/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions &&
 * limitations under the License.
 */

jest.mock('fs');
jest.mock('path');
jest.mock('../../../src/logger', () => {
    const mLogger = {
        printError: jest.fn(),
        printInfo: jest.fn(),
        getInstance: jest.fn((): any => mLogger)
    } as any;
    return {
        Logger: mLogger,
        LogDataFactory: {
            newInstance: jest.fn(() => ({
                code: '001', description: '', cause: '', position: '', solutions: [], moreInfo: {}
            }))
        }
    };
});
jest.mock('../../../src/plugins/plugins_driver', () => {
    const mPluginDriver = {
        initPlugins: jest.fn(),
        getInstance: jest.fn((): any => mPluginDriver)
    } as any;
    return { PluginDriver: mPluginDriver };
});
jest.mock('../../../src/pre_define', () => ({
    LIBARKTS_PATH_FROM_SDK: 'koala',
    PANDA_SDK_PATH_FROM_SDK: 'panda',
    PROJECT_BUILD_CONFIG_FILE: 'projectionConfig.json'
}));
jest.mock('../../../src/util/utils', () => ({
    isLinux: jest.fn(() => false),
    isMac: jest.fn(() => false),
    isWindows: jest.fn(() => false)
}));

const fakeArkts = {
    Config: { create: jest.fn(() => ({ peer: 'peer' })) },
    Context: {
        createFromString: jest.fn(() => ({ program: {}, peer: 'peer' })),
        createFromStringWithHistory: jest.fn(() => ({ program: {}, peer: 'peer' }))
    },
    proceedToState: jest.fn(),
    Es2pandaContextState: { ES2PANDA_STATE_PARSED: 1, ES2PANDA_STATE_CHECKED: 2 },
    generateTsDeclarationsFromContext: jest.fn(),
    destroyConfig: jest.fn(),
    EtsScript: { fromContext: jest.fn(() => ({})) }
};
const fakeArktsGlobal = {
    es2panda: {
        _SetUpSoPath: jest.fn(),
        _DestroyContext: jest.fn((pandaSDKPath: string) => {
            return;
        }),
    },
    filePath: '',
    config: '',
    compilerContext: { program: {}, peer: 'peer' }
};

jest.mock('../../../src/init/init_koala_modules', () => ({
    initKoalaModules: jest.fn((buildConfig) => {
        const fakeKoala = {
            arkts: fakeArkts,
            arktsGlobal: fakeArktsGlobal
        };
        fakeKoala.arktsGlobal.es2panda._SetUpSoPath(buildConfig.pandaSdkPath);

        buildConfig.arkts = fakeKoala.arkts;
        buildConfig.arktsGlobal = fakeKoala.arktsGlobal;
        return fakeKoala;
    })
}));

beforeEach(() => {
    jest.resetModules();
    jest.clearAllMocks();
    require('path').resolve.mockImplementation((...args: string[]) => args.join('/'));
    require('path').join.mockImplementation((...args: string[]) => args.join('/'));
    require('path').basename.mockImplementation((p: string) => p.split('/').pop());
    require('fs').existsSync.mockReturnValue(true);
    require('fs').mkdirSync.mockImplementation(() => { });
    require('fs').writeFileSync.mockImplementation(() => { });
    require('fs').readFileSync.mockImplementation(() => JSON.stringify({}));
    process.env = { ...process.env };
});

//Don't change or merge following tests even if they seem duplicate.
//Every test is for different scenarios and should be kept separate for clarity, avoiding mock issues.
describe('test processBuildConfig in different scenarios', () => {
    const buildConfigBase = {
        buildSdkPath: '/sdk',
        pandaSdkPath: undefined,
        cachePath: '/cache',
        abcLinkerPath: '/abc/linker',
        dependencyAnalyzerPath: '/dep/analyzer',
        frameworkMode: false,
        packageName: 'pkg'
    };

    test('process build config && call all init functions', () => {
        jest.doMock('/sdk/koala', () => ({
            arkts: fakeArkts,
            arktsGlobal: fakeArktsGlobal
        }), { virtual: true });
        const { processBuildConfig } = require('../../../src/init/process_build_config');
        const config = processBuildConfig({ ...buildConfigBase });
        const { BUILD_MODE } = require('../../../src/types');
        expect(config.pandaSdkPath).toBe('/sdk/panda');
        expect(config.buildMode).toBe(BUILD_MODE.RELEASE);
        expect(require('../../../src/plugins/plugins_driver').PluginDriver.initPlugins).toHaveBeenCalled();
        expect(config.arkts).toBe(fakeArkts);
        expect(config.arktsGlobal).toBe(fakeArktsGlobal);
        expect(fakeArktsGlobal.es2panda._SetUpSoPath).toHaveBeenCalledWith('/sdk/panda');
    });

    test('create cache dir && config file if not exist', () => {
        require('fs').existsSync.mockImplementation((p: string) => {
            if (p === '/cache') return false;
            if (p === '/cache/projectionConfig.json') return false;
            return true;
        });
        const { processBuildConfig } = require('../../../src/init/process_build_config');
        processBuildConfig({ ...buildConfigBase });
        expect(require('fs').mkdirSync).toHaveBeenCalledWith('/cache', { recursive: true });
        expect(require('fs').writeFileSync).toHaveBeenCalledWith('/cache/projectionConfig.json', expect.any(String));
    });

    test('overwrite config if config file exists but not equal', () => {
        const { processBuildConfig } = require('../../../src/init/process_build_config');
        require('fs').existsSync.mockImplementation((p: string) => true);
        require('fs').readFileSync.mockImplementation(() => JSON.stringify({ foo: 1 }));
        processBuildConfig({ ...buildConfigBase, foo: 2 });
        expect(require('fs').writeFileSync).toHaveBeenCalledWith('/cache/projectionConfig.json', expect.any(String));
    });

    test('not overwrite config if config file exists && is equal', () => {
        require('fs').existsSync.mockImplementation((p: string) => true);
        const expectedConfig = {
            ...buildConfigBase,
            pandaSdkPath: '/sdk/panda',
            isBuildConfigModified: true,
            buildMode: 'Release'
        };
        const { processBuildConfig } = require('../../../src/init/process_build_config');
        require('fs').readFileSync.mockImplementation(() => JSON.stringify(expectedConfig));
        jest.clearAllMocks();
        processBuildConfig({ ...buildConfigBase });
        expect(require('fs').writeFileSync).not.toHaveBeenCalled();
        expect(require('../../../src/logger').Logger.printInfo).toHaveBeenCalledWith(
            'projectionConfig.json is up to date.');
    });

    test('print error if abcLinkerPath not exist', () => {
        require('fs').existsSync.mockImplementation((p: string) => {
            if (p === '/abc/linker') return false;
            return true;
        });
        const { processBuildConfig } = require('../../../src/init/process_build_config');
        processBuildConfig({ ...buildConfigBase });
        expect(require('../../../src/logger').Logger.printError).toHaveBeenCalled();
    });

    test('print error if dependencyAnalyzerPath not exist && not frameworkMode', () => {
        require('fs').existsSync.mockImplementation((p: string) => {
            if (p === '/dep/analyzer') return false;
            return true;
        });
        const { processBuildConfig } = require('../../../src/init/process_build_config');
        processBuildConfig({ ...buildConfigBase });
        expect(require('../../../src/logger').Logger.printError).toHaveBeenCalled();
    });

    test('not print error if dependencyAnalyzerPath not exist but frameworkMode is true', () => {
        require('fs').existsSync.mockImplementation((p: string) => {
            if (p === '/dep/analyzer') return false;
            return true;
        });
        const { processBuildConfig } = require('../../../src/init/process_build_config');
        processBuildConfig({ ...buildConfigBase, frameworkMode: true });
        expect(require('../../../src/logger').Logger.printError).not.toHaveBeenCalled();
    });

    test('set DYLD_LIBRARY_PATH on Mac', () => {
        jest.resetModules();
        require('../../../src/util/utils').isMac.mockReturnValue(true);
        const { processBuildConfig, initBuildEnv } = require('../../../src/init/process_build_config');
        const config = { ...buildConfigBase, pandaSdkPath: '/sdk/panda' };
        process.env.PATH = '/usr/bin';
        require('path').resolve.mockImplementation((...args: string[]) => args.filter(Boolean).join('/'));
        initBuildEnv(config);
        expect(process.env.DYLD_LIBRARY_PATH).toContain('/sdk/panda/lib');
    });

    test('use KOALA_WRAPPER_PATH env if set', () => {
        process.env.KOALA_WRAPPER_PATH = '/custom/koala';
        jest.doMock('/custom/koala', () => ({
            arkts: fakeArkts,
            arktsGlobal: fakeArktsGlobal
        }), { virtual: true });
        const { processBuildConfig } = require('../../../src/init/process_build_config');
        const config = processBuildConfig({ ...buildConfigBase });
        expect(config.arkts).toBe(fakeArkts);
        expect(config.arktsGlobal).toBe(fakeArktsGlobal);
        expect(fakeArktsGlobal.es2panda._SetUpSoPath).toHaveBeenCalled();
        delete process.env.KOALA_WRAPPER_PATH;
    });

    test('throw if koala wrapper require fails', () => {
        jest.unmock('../../../src/init/init_koala_modules');
        jest.resetModules();

        process.env.KOALA_WRAPPER_PATH = '/bad/koala';
        jest.doMock('/bad/koala', () => { throw new Error('fail'); }, { virtual: true });
        const { processBuildConfig } = require('../../../src/init/process_build_config');
        expect(() => processBuildConfig({ ...buildConfigBase })).toThrow();
        delete process.env.KOALA_WRAPPER_PATH;
    });


});
