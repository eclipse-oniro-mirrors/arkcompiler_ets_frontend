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

export enum BuiltinProblem {
  LimitedThisArg = 'ThisArg',
  SymbolIterator = 'SymbolIterator',
  BuiltinNoCtorFunc = 'BuiltinNoCtorFunc',
  BuiltinNoPropertyDescriptor = 'NoPropertyDescriptor',
  MissingAttributes = 'MissingAttributes',
  BuiltinNewCtor = 'BuiltinNewCtor',
  BuiltinFinalClass = 'BuiltinFinalClass',
  BuiltinNarrowTypes = 'BuiltinNarrowTypes',
  BuiltinDisableApi = 'BuiltinDisableApi',
  BuiltinIteratorResultValue = 'BuiltinIteratorResultValue'
}

export const SYMBOL_ITERATOR: string = 'Symbol.iterator';

export const GET_OWN_PROPERTY_NAMES_TEXT: string = 'Object.getOwnPropertyNames';

export const BUILTIN_DISABLE_CALLSIGNATURE = [
  'AggregateError',
  'Array',
  'Array',
  'BigInt',
  'Boolean',
  'Date',
  'Error',
  'EvalError',
  'Number',
  'RangeError',
  'ReferenceError',
  'RegExp',
  'RegExp',
  'RegExp',
  'String',
  'SyntaxError',
  'TypeError',
  'URIError'
];

export const BUILTIN_CONSTRUCTORS = ['Boolean', 'Number', 'Object', 'String'];

export const COLLECTION_TYPES = new Set(['Map', 'Set', 'WeakMap', 'WeakSet']);

export const COLLECTION_METHODS = new Set(['add', 'delete', 'get', 'has', 'set']);

export const BUILTIN_TYPE = 'BuiltinApi';
export const BuiltinProblemInfos = new Map<string, number>([
  [BuiltinProblem.BuiltinNoCtorFunc, FaultID.BuiltinNoCtorFunc],
  [BuiltinProblem.BuiltinNoPropertyDescriptor, FaultID.NoPropertyDescriptor],
  [BuiltinProblem.BuiltinNarrowTypes, FaultID.BuiltinNarrowTypes],
  [BuiltinProblem.BuiltinDisableApi, FaultID.BuiltinDisableApi],
  [BuiltinProblem.BuiltinIteratorResultValue, FaultID.BuiltinIteratorResultValue]
]);
export const BUILTIN_CONSTRUCTOR_API_NAME: string = 'builtin_constructor';
export const BUILTIN_CONSTRUCTOR_API_TYPE = ['CallSignature'];
export const BUILTIN_CALLSIGNATURE_NEWCTOR = [
  'AggregateError',
  'Array',
  'Boolean',
  'Date',
  'Error',
  'Number',
  'String',
  'WeakMap',
  'Object'
];
