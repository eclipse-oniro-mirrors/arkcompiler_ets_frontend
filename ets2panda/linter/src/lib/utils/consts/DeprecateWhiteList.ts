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

export const DEPRECATE_TYPE = 'DeprecatedApi';
export enum DeprecateProblem {
  NoDeprecatedApi = 'NoDeprecatedApi'
}

export enum DEPRECATE_CHECK_KEY {
  PARENT_NAME = 'parentName',
  PARAM_SET = 'parameters',
  RETURN_TYPE = 'returnType',
  FILE_NAME = 'fileName'
}

export const DEPRECATE_UNNAMED = 'unnamed';
