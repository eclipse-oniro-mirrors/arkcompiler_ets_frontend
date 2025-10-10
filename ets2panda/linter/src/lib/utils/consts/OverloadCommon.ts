/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import sdkOverloadJson from '../../data/SdkOverload.json';

type ApiFuncArg = {
  name: string;
  type: string;
  is_optional: boolean;
  has_default: boolean;
};

export type OverloadInfo = {
  replacement: string;
  args: ApiFuncArg[];
};

export type OverloadApiFixMap = Map<string, OverloadInfo[]>;

export function initOverloadApiFixMap(): OverloadApiFixMap {
  const fixMap: OverloadApiFixMap = new Map();
  for (const entry of sdkOverloadJson.api_list) {
    const apiInfo = entry.api_info;
    const parentName = apiInfo.parent_api?.[0]?.api_name ?? '';
    const key = `${apiInfo.api_name}::${parentName}::${entry.file_path}`;
    const info: OverloadInfo = {
      replacement: apiInfo.api_fixed_name,
      args: apiInfo.api_func_args || []
    };
    const list = fixMap.get(key) ?? [];
    list.push(info);
    fixMap.set(key, list);
  }
  return fixMap;
}

export const COMMON_OVERLOAD_METHODS = ['on', 'off', 'once'];
export const COMMON_OVERLOAD_METHOD_PARAMETERS = ['type', 'event', 'eventType', 'evt'];
export const LIST_OVERLOAD_METHOD_PARAMETERS = ['type', 'nodeType'];
export const SDK_FILE_EXTENSIONS = ['d.ts', 'd.ets'];
export const GLOBAL_KEYWORD = 'global';
export const LIST_OVERLOAD_METHODS: Set<string> = new Set([
  'on',
  'off',
  'once',
  'bindController',
  'copyDir',
  'createImageLattice',
  'createNode',
  'deleteAssets',
  'findElement',
  'getAttribute',
  'getEvent',
  'moveDir',
  'onKeyEvent'
]);
