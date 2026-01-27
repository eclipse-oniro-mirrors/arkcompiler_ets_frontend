/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

// Mock SDK type definitions for Column and Row

export declare type SpaceType = string | number | Resource;
export declare type Resource = {};

export interface ColumnOptions {
  space?: string | number;
}

export interface ColumnOptionsV2 {
  space?: SpaceType;
}

export interface ColumnAttribute {}

export interface RowOptions {
  space?: string | number;
}

export interface RowOptionsV2 {
  space?: SpaceType;
}

export interface RowAttribute {}

// Using single call signature with union type parameter to avoid CallSignature error
export declare const Column: (options?: ColumnOptions | ColumnOptionsV2) => ColumnAttribute;
export declare const Row: (options?: RowOptions | RowOptionsV2) => RowAttribute;
