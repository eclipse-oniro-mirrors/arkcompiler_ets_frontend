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
import type { ClassDeclaration } from 'typescript';

export enum ExtendedIdentifierType {
  UNKNOWN,
  CLASS,
  ERROR
}

export type ExtendedIdentifierInfo =
  | {
    type: ExtendedIdentifierType.UNKNOWN | ExtendedIdentifierType.ERROR;
  }
  | { type: ExtendedIdentifierType.CLASS; decl: ClassDeclaration };

export type ConstructorParameter = {
  name: string;
  isOptional: boolean;
  type: string;
};

export type ParameterName = string;

export type BaseClassConstructorInfo = Set<ConstructorParameter[]> | undefined;
