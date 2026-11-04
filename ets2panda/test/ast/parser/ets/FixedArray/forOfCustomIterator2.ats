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

class A {
    data: FixedArray<string> = ['a', 'b', 'c'];
    $_iterator() : CIterator | undefined {
      return new CIterator(this);
    }
}

class CIterator implements Iterator<string> {
    index = 0;
    base: A;
    constructor (base: A) {
        this.base = base;
    }
    next(): IteratorResult<string> {
        if (this.index >= this.base.data.length) {
            return {
                done: true,
                value: undefined
            }
        }
        return {
          done: this.index >= this.base.data.length,
          value: this.base.data[this.index++]
        }
    }
}

function main(): void {
    let res = "";
    let a = new A();
    for (let x of a) res += x;
    arktest.assertEQ(res, "abc");
}


/* @@? 18:15 Error Semantic error ESE0096: The return type of '$_iterator' must be a type that implements Iterator interface. */
/* @@? 46:19 Error Semantic error ESE0261: Iterator method must return an object which implements Iterator<T> */
/* @@? 46:19 Error Semantic error ESE0076: 'For-of' statement source expression is not of iterable type. */