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

export type ParenRecord = (Record<string, ParenRecord>);

export type UnionInRecord = Record<string, string | UnionInRecord>;

export type MultiDepth = Record<string, MultiDepth> | Record<string, Record<number, MultiDepth>>;

export type SwapGeneric<T,U>  = Record<string, SwapGeneric<U, T>>;

export type MixedRecordArray = string | Record<string, MixedRecordArray> | Array<MixedRecordArray>;

namespace NS {
    export type QualifiedRecord = Record<String,NS.QualifiedRecord>;
}

export type ArrayGeneric = Array<ArrayGeneric>

export type ArrayUnion = string | Array<ArrayUnion>;

export type RecordThroughArray = Record<string, Array<RecordThroughArray>>;

export type ArrayWrappingRecord = Array<Record<string, ArrayWrappingRecord>>;

export type IndirectA = IndirectB;
export type IndirectB = Record<string,IndirectA>;

export type MutA = Record<string, MutB>;
export type MutB = Record<string, MutA>;

namespace Outer {
    export type SameName = Record<string, number>;
}
export type SameName = Record<string, Outer.SameName>;
