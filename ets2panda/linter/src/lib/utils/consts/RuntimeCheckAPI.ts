/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

import type ts from 'typescript';

export type ArrayAccess = {
  pos: number;
  accessingIdentifier: 'number' | ts.Identifier | ts.Expression;
  arrayIdent: ts.Identifier;
};

export const NUMBER_LITERAL = 'number';
export const LENGTH_IDENTIFIER = 'length';
export const ANY_TYPE = 'any';

export const LENGTH_PROPERTY = 'length';
export const KEYS_METHOD = 'keys';
export const VALUES_METHOD = 'values';
export const ENTRIES_METHOD = 'entries';
export const FROM_METHOD = 'from';
export const OBJECT_GLOBAL = 'Object';
export const RECORD_TYPE = 'Record';

export const MATH_GLOBAL = 'Math';
export const MAX_METHOD = 'max';

export const ZERO_LITERAL = 0;
export const ONE_LITERAL = 1;

export const ARRAY_TYPE_SUFFIX = '[]';
export const ARRAY_TYPE_PREFIX = 'Array<';
export const ARRAY_TYPE_NAME = 'Array';

export const ARRAY_LENGTH_SOURCE_WHITE_LIST = new Set<string>([
  `${OBJECT_GLOBAL}.${KEYS_METHOD}`,
  `${OBJECT_GLOBAL}.${VALUES_METHOD}`,
  `${OBJECT_GLOBAL}.${ENTRIES_METHOD}`,
  `${ARRAY_TYPE_NAME}.${FROM_METHOD}`
]);

export enum LoopConditionChecked {
  LEFT,
  RIGHT,
  NOT_CHECKED
}

export enum CheckResult {
  SKIP,
  HAS_ARRAY_ACCES,
  CHECKED
}
