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
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { initKoalaModules, cleanKoalaModule } from '../../../src/init/init_koala_modules';
import type { BuildConfig } from '../../../src/types';

const fakeKoalaModule = {
  arkts: {},
  arktsGlobal: {
    es2panda: { _SetUpSoPath: jest.fn() },
  },
};

jest.mock('/mock/build/path/build-tools/koala-wrapper/build/lib/es2panda', () => fakeKoalaModule, { virtual: true });

describe('initKoalaModules', () => {
  let buildConfig: BuildConfig;

  beforeEach(() => {
    buildConfig = {
      buildSdkPath: '/mock/build/path',
      pandaSdkPath: '/mock/panda/path',
    } as any;

    cleanKoalaModule();
    fakeKoalaModule.arktsGlobal.es2panda._SetUpSoPath.mockClear();
  });

  it('should load koalaModule and inject into buildConfig', () => {
    const koala = initKoalaModules(buildConfig);

    expect(koala).toBe(fakeKoalaModule);
    expect(buildConfig.arkts).toBe(fakeKoalaModule.arkts);
    expect(buildConfig.arktsGlobal).toBe(fakeKoalaModule.arktsGlobal);
    expect(fakeKoalaModule.arktsGlobal.es2panda._SetUpSoPath).toHaveBeenCalledWith(buildConfig.pandaSdkPath);
  });

  it('should reuse koalaModule on subsequent calls', () => {
    const first = initKoalaModules(buildConfig);
    const second = initKoalaModules(buildConfig);

    expect(first).toBe(second);
    expect(fakeKoalaModule.arktsGlobal.es2panda._SetUpSoPath).toHaveBeenCalledTimes(1);
  });

  it('should clean koalaModule', () => {
    initKoalaModules(buildConfig);
    cleanKoalaModule();

    const koala = initKoalaModules(buildConfig);
    expect(koala).toBe(fakeKoalaModule);
  });
});
