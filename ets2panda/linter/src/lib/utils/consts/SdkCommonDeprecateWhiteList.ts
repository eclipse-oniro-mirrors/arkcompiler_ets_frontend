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

import { FaultID } from '../../Problems';

export const SDK_COMMON_TYPE = 'SdkCommonApi';
export const SdkCommonApiProblemInfos = new Map<string, number>([
  ['WhiteList', FaultID.SdkCommonApiWhiteList],
  ['BehaviorChange', FaultID.SdkCommonApiBehaviorChange],
  ['allDeprecated', FaultID.SdkCommonApiDeprecated]
]);
export const SDK_COMMON_INDEX_CLASS: Set<string> = new Set(['Stack', 'Queue', 'LinkedList', 'PlainArray', 'Buffer']);
export enum SDK_COMMON_BUFFER_API {
  apiName = 'Buffer',
  full_api = 'buffer.Buffer',
  indexof = 'indexOf'
}
export const SDK_COMMON_FUNCTIONLIKE = ['MethodSignature', 'MethodDeclaration', 'FunctionDeclaration'];
export const SDK_COMMON_PROPERTYLIKE = ['PropertyDeclaration', 'PropertySignature'];
export const SDK_COMMON_CONSTRUCTORLIKE = ['ConstructorDeclaration'];
export const SDK_COMMON_TYPEKEY = ['funlike', 'propertyLike'];

export const SDK_COMMON_SYMBOL_ITERATOR: string = 'Symbol.iterator';
export const SDK_COMMON_SYMBOL_ITERATOR_APINAME: string = '[Symbol.iterator]';
export const SDK_COMMON_TRANSFORMER: string = 'Transformer';
export const SDK_COMMON_CONSTRUCTOR: string = 'constructor';
export const SDK_COMMON_VOID: string = 'void';

export const sdkCommonAllDeprecatedTypeName: Set<string> = new Set([
  'Base64',
  'LruBuffer',
  'Scope',
  'Vector',
  'ConvertXML',
  'ConvertOptions',
  'URLSearchParams'
]);
export const sdkCommonAllDeprecatedFullTypeName: Set<string> = new Set([
  'Base64',
  'LruBuffer',
  'Scope',
  'Vector',
  'ConvertXML',
  'ConvertOptions',
  'URLSearchParams',
  'util.Base64',
  'util.LruBuffer',
  'util.Scope',
  'Vector',
  'xml.ConvertXML',
  'xml.ConvertOptions',
  'url.URLSearchParams'
]);
