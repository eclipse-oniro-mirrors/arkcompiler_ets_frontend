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

import { Lsp, PathConfig } from '../src';
import path from 'path';

export const DEFAULT_PATH_CONFIG: PathConfig = {
  buildSdkPath: path.resolve('test', 'ets', 'static'),
  projectPath: path.resolve('test', 'testcases'),
  declgenOutDir: ''
};

const DEFAULT_PLUGIN_LIST: string[] = process.env.SKIP_UI_PLUGINS ? [] : ['ui-plugins', 'memo-plugins'];

export function getLsp(moduleName: string, plugins: string[] = DEFAULT_PLUGIN_LIST): Lsp {
  const bindingsPath =
    process.env.BINDINGS_PATH || path.join(DEFAULT_PATH_CONFIG.buildSdkPath, 'build-tools', 'bindings');

  const pandaLibPath =
    process.env.PANDA_LIB_PATH || path.join(DEFAULT_PATH_CONFIG.buildSdkPath, 'build-tools', 'ets2panda', 'lib');

  const pandaBinPath =
    process.env.PANDA_BIN_PATH || path.join(DEFAULT_PATH_CONFIG.buildSdkPath, 'build-tools', 'ets2panda', 'bin');

  process.env.BINDINGS_PATH = bindingsPath;
  process.env.PANDA_LIB_PATH = pandaLibPath;
  process.env.PANDA_BIN_PATH = pandaBinPath;

  return new Lsp(
    DEFAULT_PATH_CONFIG,
    undefined,
    [{ name: moduleName, moduleType: 'har', srcPath: path.resolve('test/testcases/', moduleName) }],
    plugins
  );
}

export function getRealPath(moduleName: string, fileName: string): string {
  return path.resolve('test/testcases/', moduleName, fileName);
}
