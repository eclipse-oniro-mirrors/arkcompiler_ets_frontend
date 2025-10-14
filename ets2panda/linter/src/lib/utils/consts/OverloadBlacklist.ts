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

import overloadBlacklist from '../../data/OverloadAPIBlacklist.json';

export const TYPE_LITERAL = 'type';
export const SINGLEQUOTE = "'";

export type Parameter = {
  name: string;
  type: string;
};

export type OverloadBlacklistInfo = {
  fileName: string;
  parentName: string;
  params: Parameter[];
};

export type OverloadBlacklistMap = Map<string, OverloadBlacklistInfo[]>;

export function createOverloadMapKey(apiName: string, fileName: string, parentApiName: string): string {
  return `${apiName}::${parentApiName}::${fileName}`;
}

export function initializeOverloadBlacklistMap(): OverloadBlacklistMap {
  const blacklistMap = new Map<string, OverloadBlacklistInfo[]>();

  for (const info of overloadBlacklist.api_list) {
    const api = info.api_info.api_name;
    const params = info.api_info.api_func_args;
    const fileName = info.file_path;
    const parentName = info.api_info.parent_api[0].api_name;
    const key = createOverloadMapKey(api, parentName, fileName);

    const newBlacklistInfo = {
      fileName,
      parentName,
      params
    };

    const blackListInfo = blacklistMap.get(key);
    if (!blackListInfo) {
      blacklistMap.set(key, [newBlacklistInfo]);
      continue;
    }

    blackListInfo.push(newBlacklistInfo);
    blacklistMap.set(key, blackListInfo);
  }

  return blacklistMap;
}
