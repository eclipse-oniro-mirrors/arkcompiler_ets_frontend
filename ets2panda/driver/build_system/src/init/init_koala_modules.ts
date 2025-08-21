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


import { KOALA_WRAPPER_PATH_FROM_SDK, MEMO_PLUGIN_PATH_FROM_SDK, UI_PLUGIN_PATH_FROM_SDK } from '../pre_define';
import { BuildConfig } from '../types';
import path from 'path'


let koalaModule: any;

export function initKoalaModules(buildConfig: BuildConfig) {
  if (!koalaModule) {
    const koalaWrapperPath =
      process.env.KOALA_WRAPPER_PATH ??
      path.resolve(buildConfig.buildSdkPath, KOALA_WRAPPER_PATH_FROM_SDK);

    koalaModule = require(koalaWrapperPath);
    koalaModule.arktsGlobal.es2panda._SetUpSoPath(buildConfig.pandaSdkPath);
  }

  Object.assign(buildConfig, {
    arkts: koalaModule.arkts,
    arktsGlobal: koalaModule.arktsGlobal,
  });

  return koalaModule;
}


export function initKoalaPlugins(projectConfig: BuildConfig): void {
  const uiPluginPath = path.resolve(projectConfig.buildSdkPath, UI_PLUGIN_PATH_FROM_SDK);
  const memoPluginPath = path.resolve(projectConfig.buildSdkPath, MEMO_PLUGIN_PATH_FROM_SDK);

    // TODO: need change in hvigor
    if (process.env.USE_KOALA_UI_PLUGIN) {
      projectConfig.plugins.ArkUI = uiPluginPath
    }

    if (process.env.USE_KOALA_MEMO_PLUGIN) {
      projectConfig.plugins['ArkUI-Memo'] = memoPluginPath
    }
}

export function cleanKoalaModule(): void {
  koalaModule = null;
}

// for ut
export function getKoalaModule() {
  return koalaModule;
}