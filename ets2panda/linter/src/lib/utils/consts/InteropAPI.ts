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
import type * as ts from 'typescript';

export const ARE_EQUAL = 'areEqual';
export const ARE_STRICTLY_EQUAL = 'areStrictlyEqual';
export const WRAP = 'wrap';
export const INSTANTIATE = 'instantiate';
export const LENGTH = 'length';
export const INVOKE = 'invoke';
export const INVOKE_METHOD = 'invokeMethod';
export const TO_PROMISE = 'toPromise';
export const IS_INSTANCE_OF = 'isInstanceOf';

export const REFLECT_PROPERTIES = [
  'get',
  'set',
  'has',
  'ownKeys',
  'getOwnPropertyDescriptor',
  'defineProperty',
  'deleteProperty',
  'apply',
  'construct',
  'getPrototypeOf',
  'setPrototypeOf',
  'isExtensible',
  'preventExtensions'
];

export const OBJECT_PROPERTIES = [
  'get',
  'set',
  'has',
  'hasOwn',
  'ownKeys',
  'keys',
  'getOwnPropertyDescriptor',
  'getOwnPropertyDescriptors',
  'getOwnPropertyName',
  'defineProperty',
  'deleteProperty',
  'apply',
  'construct',
  'getPrototypeOf',
  'setPrototypeOf',
  'isExtensible',
  'isFrozen',
  'isSealed'
];

export const USE_STATIC = '\'use static\'';
export const OBJECT_LITERAL = 'Object';
export const REFLECT_LITERAL = 'Reflect';
export const NONE = 'none';
export type ForbiddenAPICheckResult = 'Object' | 'Reflect' | 'none';
export const GET_PROPERTY = 'getProperty';
export const SET_PROPERTY = 'setProperty';
export const TO_NUMBER = 'toNumber';

export enum InteropType {
  TS = 'TS',
  JS = 'JS',
  LEGACY = '1.0',
  NONE = 'none'
}

export type IdentifierAndArguments = {
  ident: undefined | ts.Identifier;
  args: ts.NodeArray<ts.Expression> | undefined;
};

export type IncrementDecrementNodeInfo = {
  varAssignText: string;
  addOrDecrOperation: ts.BinaryExpression;
};
