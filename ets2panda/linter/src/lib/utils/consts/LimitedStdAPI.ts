/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

const LIMITED_STD_SYMBOL_API = [
  // properties
  'asyncIterator',
  'description',
  'hasInstance',
  'isConcatSpreadable',
  'match',
  'matchAll',
  'replace',
  'search',
  'species',
  'split',
  'toPrimitive',
  'toStringTag',
  'unscopables',

  // methods
  'for',
  'keyFor',
  'toString',
  'valueOf'
];

export const ALLOWED_STD_SYMBOL_API = ['iterator'];

export const LIMITED_STD_OBJECT_API = [
  // properties
  '__proto__',

  // methods
  '__defineGetter__',
  '__defineSetter__',
  '__lookupGetter__',
  '__lookupSetter__',
  'assign',
  'create',
  'defineProperties',
  'defineProperty',
  'freeze',
  // was relaxed but revert
  'fromEntries',
  'getOwnPropertyDescriptor',
  'getOwnPropertyDescriptors',
  'getOwnPropertySymbols',
  'getPrototypeOf',
  'hasOwnProperty',
  'is',
  'isExtensible',
  'isFrozen',
  'isPrototypeOf',
  'isSealed',
  'preventExtensions',
  'propertyIsEnumerable',
  'seal',
  'setPrototypeOf'
];

export const LIMITED_STD_PROXYHANDLER_API = [
  // properties

  // methods
  'apply',
  'construct',
  'defineProperty',
  'deleteProperty',
  'get',
  'getOwnPropertyDescriptor',
  'getPrototypeOf',
  'has',
  'isExtensible',
  'ownKeys',
  'preventExtensions',
  'set',
  'setPrototypeOf'
];

export const LIMITED_STD_REFLECT_API = [
  // properties

  // methods
  'apply',
  'construct',
  'defineProperty',
  'deleteProperty',
  'getOwnPropertyDescriptor',
  'getPrototypeOf',
  'isExtensible',
  'preventExtensions',
  'setPrototypeOf'
];

const LIMITED_STD_FUNCTION_API_APPLY_CALL = [
  // properties

  // methods
  'apply',
  'call'
];

const LIMITED_STD_FUNCTION_API_BIND = [
  // properties

  // methods
  'bind'
];

export const LIMITED_STD_GLOBAL_API = [
  // properties

  // methods
  'eval'
];

const STD_SYMBOL_ENTRY = [{ api: LIMITED_STD_SYMBOL_API, faultId: FaultID.SymbolType }];
const STD_OBJECT_ENTRY = [{ api: LIMITED_STD_OBJECT_API, faultId: FaultID.LimitedStdLibApi }];
const STD_PROXYHANDLER_ENTRY = [{ api: LIMITED_STD_PROXYHANDLER_API, faultId: FaultID.LimitedStdLibApi }];
const STD_REFLECT_ENTRY = [{ api: LIMITED_STD_REFLECT_API, faultId: FaultID.LimitedStdLibApi }];
const STD_FUNCTION_ENTRY = [
  { api: LIMITED_STD_FUNCTION_API_APPLY_CALL, faultId: FaultID.FunctionApplyCall },
  { api: LIMITED_STD_FUNCTION_API_BIND, faultId: FaultID.FunctionBindError }
];
const STD_GLOBAL_ENTRY = [{ api: LIMITED_STD_GLOBAL_API, faultId: FaultID.LimitedStdLibApi }];

export type LimitedStdLibApiEntry = { api: string[]; faultId: FaultID };

export const LIMITED_STD_API = new Map<string | undefined, LimitedStdLibApiEntry[]>([
  [undefined, STD_GLOBAL_ENTRY],
  ['Object', STD_OBJECT_ENTRY],
  ['ObjectConstructor', STD_OBJECT_ENTRY],
  ['Reflect', STD_REFLECT_ENTRY],
  ['ProxyHandler', STD_PROXYHANDLER_ENTRY],
  ['Symbol', STD_SYMBOL_ENTRY],
  ['SymbolConstructor', STD_SYMBOL_ENTRY],
  ['Function', STD_FUNCTION_ENTRY],
  ['CallableFunction', STD_FUNCTION_ENTRY]
]);

export const MODULE_IMPORTS: Record<string, string[]> = {
  '@kit.ArkTS': ['taskpool', 'ArkTSUtils', 'process', 'collections'],
  '@ohos.process': ['process'],
  '@ohos.taskpool': ['taskpool'],
  '@arkts.utils': ['ArkTSUtils'],
  '@arkts.collections': ['collections']
};

export const ARKTSUTILS_MODULES = ['@arkts.utils', '@ohos.process', '@kit.ArkTS'];
export const ARKTSUTILS_LOCKS_MEMBER = 'locks';

export const OBJECT_PUBLIC_API_METHOD_SIGNATURES = new Map<string, string>([
  ['toString', '(): string'],
  ['toLocaleString', '(): string'],
  ['valueOf', '(): Object'],
  ['hasOwnProperty', '(v: PropertyKey): boolean'],
  ['isPrototypeOf', '(v: Object): boolean'],
  ['propertyIsEnumerable', '(v: PropertyKey): boolean']
]);
export const ARKTSUTILS_PROCESS_MEMBER = 'process';

export const PROCESS_DEPRECATED_INTERFACES = [
  'isAppUid',
  'getUidForName',
  'getThreadPriority',
  'getSystemConfig',
  'getEnvironmentVar',
  'exit',
  'kill'
];
