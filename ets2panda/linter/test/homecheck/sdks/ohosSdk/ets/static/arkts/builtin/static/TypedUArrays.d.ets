'use static';
/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License"),
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

export class Uint8ClampedArray {

    public get length(): int;

    public keys(): IterableIterator<int>;

    public copyWithin(target: int, start: int, end?: int): Uint8ClampedArray;

    public indexOf(searchElement: int): int;

    public indexOf(searchElement: int, fromIndex: int): int;

    public lastIndexOf(searchElement: int): int;

    public lastIndexOf(searchElement: int, fromIndex: int): int;

    public fill(value: int, start?: int, end?: int): this;

    public slice(begin?: int, end?: int): Uint8ClampedArray;

    public subarray(begin?: int, end?: int): Uint8ClampedArray;
}

export class Uint8Array {

    public get length(): int;

    public keys(): IterableIterator<int>;

    public copyWithin(target: int, start: int, end?: int): Uint8Array;

    public indexOf(searchElement: int): int;

    public indexOf(searchElement: int, fromIndex: int): int;

    public lastIndexOf(searchElement: int): int;

    public lastIndexOf(searchElement: int, fromIndex: int): int;

    public fill(value: int, start?: int, end?: int): this;

    public slice(begin?: int, end?: int): Uint8Array;

    public subarray(begin?: int, end?: int): Uint8Array;
}

export class Uint16Array {

    public get length(): int;

    public keys(): IterableIterator<int>;

    public copyWithin(target: int, start: int, end?: int): Uint16Array;

    public indexOf(searchElement: int): int;

    public indexOf(searchElement: int, fromIndex: int): int;

    public lastIndexOf(searchElement: int): int;

    public lastIndexOf(searchElement: int, fromIndex: int): int;

    public fill(value: int, start?: int, end?: int): Uint16Array;

    public with(index: int, value: int): Uint16Array;

    public slice(begin?: int, end?: int): Uint16Array;

    public subarray(begin?: int, end?: int): Uint16Array;
}

export class Uint32Array {

    public get length(): int;

    public keys(): IterableIterator<int>;

    public copyWithin(target: int, start: int, end?: int): Uint32Array;

        public indexOf(searchElement: int): int;

    public indexOf(searchElement: int, fromIndex: int): int;

    public lastIndexOf(searchElement: int): int;

    public lastIndexOf(searchElement: int, fromIndex: int): int;

    public fill(value: long, start?: int, end?: int): Uint32Array;

    public slice(begin?: int, end?: int): Uint32Array;

    public subarray(begin?: int, end?: int): Uint32Array;
}

export class BigInt64Array {

    public get length(): int;

    public keys(): IterableIterator<int>;

    public copyWithin(target: int, start: int, end?: int): BigInt64Array;

    public indexOf(searchElement: int): int;

    public indexOf(searchElement: int, fromIndex: int): int;

    public indexOf(searchElement: BigInt, fromIndex?: int): int;

    public lastIndexOf(searchElement: int): int;

    public lastIndexOf(searchElement: BigInt): int;

    public lastIndexOf(searchElement: int, fromIndex: int): int;

    public lastIndexOf(searchElement: BigInt, fromIndex: int | undefined): int;

    public with(index: int, value: long): BigInt64Array;

    public with(index: int, value: BigInt): BigInt64Array;

    public fill(value: long, start?: int, end?: int): BigInt64Array;

    public fill(value: BigInt, start?: int, end?: int): BigInt64Array;

    public slice(begin?: int, end?: int): BigInt64Array;

    public subarray(begin?: int, end?: int): BigInt64Array;
}
