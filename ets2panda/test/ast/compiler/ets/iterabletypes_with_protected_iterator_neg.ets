/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

class AIterator implements Iterator<string> {
    index = 0;
    base: A
    constructor (base: A) {
        this.base = base;
    }
    next(): IteratorResult<string> {
        return {
            done: this.index >= this.base.data.length,
            value: this.index >= this.base.data.length ? undefined : this.base.data[this.index++]
        }
    }
}

class A {
    data: string[] = ['t', 'e', 's', 't'];

    protected $_iterator() {
        return new AIterator(this);
    }
}

function main(): int {
    let a = new A;
    for(let it of a) {
        arktest.assertEQ(it, a.data[idx]);
        ++idx;
    }
    return 0;
}

/* @@? 40:19 Error TypeError: Cannot find iterator method with the required signature. */
/* @@? 40:19 Error TypeError: 'For-of' statement source expression is not of iterable type. */
