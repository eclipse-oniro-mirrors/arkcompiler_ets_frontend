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

class Array<T> implements ReadonlyArray<T>, Iterable<T> {

    constructor(first: T, ...d: T[]);

    constructor(arrayLen: int, initializer: (index: int) => T);

    $_iterator(): IterableIterator<T>;

    get length(): int;

    $_get(idx: int): T;

    $_set(idx: int, val: T): void;

    static $_invoke<T>(...items: T[]): Array<T>;

    static from<T>(arr: FixedArray<T>): Array<T>;

    static from<T>(arr: ArrayLike<T>): Array<T>;

    static from<T>(iterable: ArrayLike<T> | Iterable<T>): Array<T>;

    static from<T, U>(values: FixedArray<T>, mapfn: (v: T, k: int) => U): Array<U>;

    static from<T, U>(iterable: ArrayLike<T> | Iterable<T>, mapfn: (v: T, k: int) => U): Array<U>;

    static isArray(o: Any): boolean;

    forEach(callbackfn: (value: T, index: int, array: Array<T>) => void): void;

    public static create<T>(arrayLength: int, initialValue: T): Array<T>;

    public sort(comparator?: (a: T, b: T) => int): this;

    public toSorted(): Array<T>;

    public toSorted(comparator: (a: T, b: T) => int): Array<T>;

    public keys(): IterableIterator<int>;

    public entries(): IterableIterator<[int, T]>;

    set length(newLen: int);

    public push(...val: T[]): int;

    public splice(start: int, del: int | undefined, ...items: T[]): Array<T>;

    public unshift(...values: T[]): int;

    public findIndex(predicate: (value: T, index: int, array: Array<T>) => boolean): int;

    public filter(predicate: (value: T, index: int, array: Array<T>) => boolean): Array<T>;

    public find(predicate: (value: T, index: int, array: Array<T>) => boolean): T | undefined;

    public findLast(predicate: (elem: T, index: int, array: Array<T>) => boolean): T | undefined;

    public every(predicate: (value: T, index: int, array: Array<T>) => boolean): boolean;

    public some(predicate: (value: T, index: int, array: Array<T>) => boolean): boolean;

    public findLastIndex(predicate: (element: T, index: int, array: Array<T>) => boolean): int;

    public reduce(callbackfn: (previousValue: T, currentValue: T, index: int, array: Array<T>) => T): T;

    public reduce<U = T>(callbackfn: (previousValue: U, currentValue: T, index: int, array: Array<T>) => U,
                         initialValue: U): U;

    public reduceRight<U>(callbackfn: (previousValue: U, currentValue: T, index: int, array: Array<T>) => U,
                          initialValue: U): U;

    public map<U>(callbackfn: (value: T, index: int, array: Array<T>) => U): Array<U>;

    public reduceRight(callbackfn: (previousValue: T, currentValue: T, index: int, array: Array<T>) => T): T;
}
