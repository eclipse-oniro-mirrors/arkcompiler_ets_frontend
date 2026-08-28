'use static'
/**
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

interface Holder<T> {}
type MyAlias<T> = Holder<T>;

export type DirectRecord = Record<string, DirectRecord>;

export type MixedUnion = string | Record<string, MixedUnion> | Holder<MixedUnion>;

export type NestedRecord = Record<string, Record<number, NestedRecord>>;

export type TwoBranches = Record<string, TwoBranches> | Record<number, TwoBranches>;

export type GenericAlias<T> = Record<string, GenericAlias<T>>;

export type AliasSelfRef = MyAlias<AliasSelfRef>;

export type ThroughHolder = Record<string, Holder<ThroughHolder>>;

export type HolderDirect = Holder<HolderDirect>;

export type HolderInUnion = string | Holder<HolderInUnion>;

export type RecordThroughHolder = Record<string, Holder<RecordThroughHolder>>;

export type HolderWrappingRecord = Holder<Record<string, HolderWrappingRecord>>;

export type NonRecursive = Record<string, number>;

export type DirectSelfRef = DirectSelfRef;

export type RecordThroughTuple = Record<string, [RecordThroughTuple, number]>;

export type RecordThroughFunc = Record<string, () => RecordThroughFunc>;

export type MixedDeferred = Record<string, Holder<MixedDeferred> | [MixedDeferred] | (() => MixedDeferred)>;

export type ArrayShorthand = ArrayShorthand[];
