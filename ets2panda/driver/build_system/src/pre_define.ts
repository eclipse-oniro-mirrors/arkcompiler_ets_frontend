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

export const ARKTSCONFIG_JSON_FILE: string = 'arktsconfig.json';
export const MERGED_CLUSTER_FILE: string = 'cluster.abc';
export const MERGED_ABC_FILE: string = 'modules_static.abc';
export const LINKER_INPUT_FILE: string = 'fileInfo.txt';
export const DEP_ANALYZER_INPUT_FILE: string = 'dependencyFileInfo.txt';
export const DEP_ANALYZER_OUTPUT_FILE: string = 'dependency.json';
export const DEP_ANALYZER_DIR: string = 'dep_analyzer';
export const PROJECT_BUILD_CONFIG_FILE: string = 'projectionConfig.json';
export const STATIC_RECORD_FILE: string = 'static.Record.d.ts';
export const FILE_HASH_CACHE: string = 'hash_cache.json';

export const DECL_ETS_SUFFIX: string = '.d.ets';
export const DECL_TS_SUFFIX: string = '.d.ts';
export const ETS_SUFFIX: string = '.ets';
export const TS_SUFFIX: string = '.ts';
export const ABC_SUFFIX: string = '.abc';

export enum LANGUAGE_VERSION {
    ARKTS_1_2 = '1.2',
    ARKTS_1_1 = '1.1',
    ARKTS_HYBRID = 'hybrid',
};

export const PANDA_SDK_PATH_FROM_SDK: string = './build-tools/ets2panda';
export const SYSTEM_SDK_PATH_FROM_SDK: string = './';
export const KIT_CONFIGS_PATH_FROM_SDK: string = '../dynamic/build-tools/ets-loader/kit_configs';

export const DEFAULT_WORKER_NUMS: number = 6;

export const ETS_1_1 = 'dynamic';
export const ETS_1_1_INTEROP = 'dynamic-interop';

export const sdkConfigPrefix = 'ohos|system|kit|arkts';
export const NATIVE_MODULE: Set<string> = new Set(
    ['system.app', 'ohos.app', 'system.router', 'system.curves', 'ohos.curves', 'system.matrix4', 'ohos.matrix4']);

export const ARKTS_MODULE_NAME: string = 'arkts';

export const KITS: string = 'kits';
export const API: string = 'api';
export const ARKTS: string = 'arkts';
export const COMPONENT: string = 'component';

export const DYNAMIC_PREFIX: string = 'dynamic/';

export const STATIC_RECORD_FILE_CONTENT: string = `// generated for static Record
export type Record<K extends keyof any, T> = {
  [P in K]: T;
};
`;
// Koala experimental variables
export const KOALA_WRAPPER_PATH_FROM_SDK: string = process.env.USE_KOALA_LIBARKTS ? './build-tools/ui2abc/libarkts/lib/libarkts.js' : './build-tools/koala-wrapper/build/lib/es2panda'
export const UI_PLUGIN_PATH_FROM_SDK: string = './build-tools/ui2abc/ui-plugin/lib/entry.js'
export const MEMO_PLUGIN_PATH_FROM_SDK: string = './build-tools/ui2abc/memo-plugin/lib/entry.js'

export const CLUSTER_FILES_TRESHOLD: number = 100;
