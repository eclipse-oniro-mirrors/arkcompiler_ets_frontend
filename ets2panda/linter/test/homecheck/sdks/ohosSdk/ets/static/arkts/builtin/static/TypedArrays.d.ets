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

export class Int8Array {

    public copyWithin(target: int, start: int, end?: int): Int8Array;

    public indexOf(searchElement: int): int;

    public indexOf(searchElement: int, fromIndex: int): int;

    public keys(): IterableIterator<int>;

    public lastIndexOf(searchElement: int): int;

    public lastIndexOf(searchElement: int, fromIndex: int): int;

    public fill(value: byte, start?: int, end?: int): this;

    public reverse(): Int8Array;

    public slice(begin: int): Int8Array;

    public slice(begin?: int, end?: int): Int8Array;

    public subarray(begin?: int, end?: int): Int8Array;
}

export class Int16Array {

    public copyWithin(target: int, start: int, end?: int): Int16Array;

    public indexOf(searchElement: int): int;

    public indexOf(searchElement: int, fromIndex: int): int;

    public keys(): IterableIterator<int>;

    public lastIndexOf(searchElement: int): int;

    public lastIndexOf(searchElement: int, fromIndex: int): int;

    public fill(value: byte, start?: int, end?: int): this;

    public reverse(): Int16Array;

    public slice(begin: int): Int16Array;

    public slice(begin?: int, end?: int): Int16Array;

    public subarray(begin?: int, end?: int): Int16Array;
}

export class Int32Array {

    public copyWithin(target: int, start: int, end?: int): Int32Array;

    public indexOf(searchElement: int): int;

    public indexOf(searchElement: int, fromIndex: int): int;

    public keys(): IterableIterator<int>;

    public lastIndexOf(searchElement: int): int;

    public lastIndexOf(searchElement: int, fromIndex: int): int;

    public fill(value: int, start?: int, end?: int): this;

    public reverse(): Int32Array;

    public slice(begin: int): Int32Array;

    public slice(begin?: int, end?: int): Int32Array;

    public subarray(begin?: int, end?: int): Int32Array;
}

export class BigInt64Array {

    public copyWithin(target: int, start: int, end?: int): BigInt64Array;

    public indexOf(searchElement: int): int;

    public indexOf(searchElement: int, fromIndex: int): int;

    public keys(): IterableIterator<int>;

    public lastIndexOf(searchElement: int): int;

    public lastIndexOf(searchElement: int, fromIndex: int): int;

    public fill(value: long, start?: int, end?: int): this;

    public fill(value: BigInt, start?: int, end?: int): this;

    public reverse(): BigInt64Array;

    public slice(begin: int): BigInt64Array;

    public slice(begin?: int, end?: int): BigInt64Array;

    public subarray(begin?: int, end?: int): BigInt64Array;
}

export class Float32Array {

    public copyWithin(target: int, start: int, end?: int): Float32Array;

    public indexOf(searchElement: int): int;

    public indexOf(searchElement: int, fromIndex: int): int;

    public keys(): IterableIterator<int>;

    public lastIndexOf(searchElement: int): int;

    public lastIndexOf(searchElement: int, fromIndex: int): int;

    public fill(value: float, start?: int, end?: int): this;

    public fill(value: double, start?: int, end?: int): this;

    public reverse(): Float32Array;

    public slice(begin: int): Float32Array;

    public slice(begin?: int, end?: int): Float32Array;

    public subarray(begin?: int, end?: int): Float32Array;
}

export class Float64Array {

    public copyWithin(target: int, start: int, end?: int): Float64Array;

    public indexOf(searchElement: int): int;

    public indexOf(searchElement: int, fromIndex: int): int;

    public keys(): IterableIterator<int>;

    public lastIndexOf(searchElement: int): int;

    public lastIndexOf(searchElement: int, fromIndex: int): int;

    public fill(value: double, start?: int, end?: int): this;

    public reverse(): Float64Array;

    public slice(begin: int): Float64Array;

    public slice(begin?: int, end?: int): Float64Array;

    public subarray(begin?: int, end?: int): Float64Array;
}
