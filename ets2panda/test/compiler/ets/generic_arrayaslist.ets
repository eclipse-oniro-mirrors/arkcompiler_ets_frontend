/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

// This contains the initial implementation of the generic ArrayAsList
// class. After development into the stdlib, this test can be removed.

interface Listt <T>  {
    pushFront(e: T): void;
    popFront(): T;
    pushBack(e: T): void;
    popBack(): T;

    size(): int;
    at(index: int): T;
    has(e: T): boolean;

    forEach(fn: (e: T) => T): Listt<T> | null;
    map<U>(fn: (e: T) => U): Listt<U>;
    fold(combine: (lhs: T, rhs: T) => T): T | null;
    foldWith<U>(combine: (lhs: U, rhs: T) => U, initVal: U): U;
    filter(filterCond: (e: T) => boolean) : Listt<T> | null;
    sort(comparator: (lhs: T, rhs: T) => boolean) : Listt<T> | null;
}

class ArrayAsListt<T> implements Listt<T> {

    private init(capacity: int, val: T | null): void {
        this.data = new (T | undefined)[capacity];
        for (let i = 0; i < this.data.length; ++i) {
        }
        this.curSize = capacity;
    }

    constructor(capacity: int, val: T) {
        this.init(capacity, val);
    }

    constructor() {
        this.init(0, null);
    }

    constructor(capacity: int) {
        this.init(capacity, null);
    }

    public reserve(capacity: int): void {
        if (this.data.length < capacity) {
            let newData = new (T | undefined)[capacity];
            for (let i = 0; i < this.curSize; ++i) {
               newData[i] = this.data[i];
            }
            this.data = newData;
        }
    }

    private getNewCapacity(currentCapacity: int): int {
        const fastGrowThreshold = 8192;
        const multiplier = 2;
        if (currentCapacity < fastGrowThreshold) {
            // Adding 4 to jump over 0
            return (currentCapacity + 4) * multiplier * 2;
        } else {
            return currentCapacity * multiplier;
        }
    }

    public override pushFront(e: T): void {
        let dst = this.data;
        if (this.curSize == this.data.length) {
            dst = new (T | undefined)[this.getNewCapacity(this.data.length.toInt())];
        }
        for (let i = this.curSize; i != 0; --i) {
            dst[i] = this.data[i-1];
        }
        this.data = dst;
        this.data[0] = e;
        ++this.curSize;
    }

    public override popFront(): T {
        arktest.assertLT(0, this.curSize, "No data to popFront from ArrayAsList!");
        let res: T = this.data[0] as T;
        for (let i = 1; i < this.curSize; ++i) {
            this.data[i-1] = this.data[i];
        }
        --this.curSize;
        return res;
    }

    public override pushBack(e: T): void {
        if (this.curSize == this.data.length) {
            let newData = new (T | undefined)[this.getNewCapacity(this.data.length.toInt())];
            for (let i = 0; i < this.curSize; ++i) {
                newData[i] = this.data[i];
            }
            this.data = newData;
        }
        this.data[this.curSize] = e;
        ++this.curSize;
    }

    public override popBack(): T {
        arktest.assertLT(0, this.curSize, "No data to popBack in ArrayAsList!");
        --this.curSize;
        return this.data[this.curSize] as T;
    }

    public override size(): int {
        return this.curSize;
    }

    public override at(index: int): T {
        return this.data[index] as T;
    }

    public override has(e: T): boolean {
        arktest.assertTrue(false)

        for (let i = 0; i < this.curSize; ++i) {
            if (this.data[i] === e) {
                return true;
            }
        }

        return false;
    }


    public override forEach(fn: (e: T) => T): Listt<T> | null {
        for (let i = 0; i < this.curSize; ++i) {
            this.data[i] = fn(this.data[i] as T);
        }
        return null;
    }

    public override map<U>(fn: (e: T) => U): Listt<U> {
        let res = new ArrayAsListt<U>();
        for (let i = 0; i < this.curSize; ++i) {
            res.pushBack(fn(this.data[i] as T));
        }
        return res;
    }

    public override fold(combine: (lhs: T, rhs: T) => T): T | null {
        if (this.curSize > 0) {
            let res = this.data[0] as T;
            for (let i = 1; i < this.curSize; ++i) {
                res = combine(res, this.data[i] as T);
            }
            return res;
        }
        try{
            throw new NoDataError();
        } catch (a){}
        return null;
    }

    public override foldWith<U>(combine: (lhs: U, rhs: T) => U, initVal: U): U {
        let res = initVal;
        for (let i = 0; i < this.curSize; ++i) {
            res = combine(res, this.data[i] as T);
        }
        return res;
    }

    public override filter(filterCond: (e: T) => boolean): ArrayAsListt<T> | null {
        let indicators = new boolean[this.data.length];
        let resAmount = 0;
        for (let i = 0; i < this.curSize; ++i) {
            indicators[i] = filterCond(this.data[i] as T);
            if (indicators[i]) {
                ++resAmount;
            }
        }
        let res = new (T | undefined)[resAmount];
        for (let i = 0, j = 0; i < this.curSize; ++i) {
            if (indicators[i]) {
                res[j] = this.data[i];
                ++j;
            }
        }
        this.data = res;
        return null;
    }

    public override sort(comparator: (lhs: T, rhs: T) => boolean): ArrayAsListt<T> | null {
        this.sortPart(this.data, 0, this.curSize, comparator);
        return null;
    }

    private sortPart(arr: (T | undefined)[], l: int, r: int, comparator: (lhs: T, rhs: T) => boolean): void {

    }

    private static partition(arr: T[], l: int, r: int, comparator: (lhs: T, rhs: T) => boolean): int {
        let last = r - 1;
        let pivot = arr[last];
        let lessInd = l - 1;
        for (let i = l; i < last; ++i) {
            if (comparator(arr[i], pivot)) {
                ++lessInd;
                let tmp = arr[i];
                arr[i] = arr[lessInd];
                arr[lessInd] = tmp;
            }
        }
        let tmp = arr[lessInd + 1];
        arr[lessInd + 1] = arr[last];
        arr[last] = tmp;
        return lessInd + 1;
    }

    private static bubbleSort(arr: T[], l: int, r: int, comparator: (lhs: T, rhs: T) => boolean): void {
        for (let i = l; i < r; ++i) {
            for (let j = i; j < r - i; ++j) {
                if (comparator(arr[j + 1], arr[j])) {
                    let tmp = arr[j + 1];
                    arr[j + 1] = arr[j];
                    arr[j] = tmp;
                }
            }
        }
    }


    private data: (T | undefined)[] = [];
    private curSize: int;
}
