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

import blacklistJson from '../../data/SdkKeywordBlacklist.json';

type ForbiddenKeywordInfo = {
  replacement: string;
  parentType: string;
  sdkFile: string;
};

type Keyword = string;
export type ForbiddenSdkApiKeywordMap = Map<Keyword, ForbiddenKeywordInfo[]>;

export function initForbiddenSdkApiKeywordsMap(): ForbiddenSdkApiKeywordMap {
  const forbiddenKeywordMap: ForbiddenSdkApiKeywordMap = new Map();
  for (const api of blacklistJson.api_list) {
    const apiInfo = api.api_info;
    const importPaths = api.import_path;

    let infoList = forbiddenKeywordMap.get(apiInfo.api_name);
    if (!infoList) {
      infoList = [];
    }

    for (const importPath of importPaths) {
      infoList.push({
        replacement: apiInfo.api_auto_fix_content,
        sdkFile: importPath,
        parentType: apiInfo.parent_api[0].api_name
      });
    }

    forbiddenKeywordMap.set(apiInfo.api_name, infoList);
  }

  return forbiddenKeywordMap;
}
