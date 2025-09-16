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

class B extends A {
    $_iterator() {
        return super.$_iterator();
    }
}

function main(): int {
    let b = new B;
    let idx = 0;
    for(let it of b) {
        arktest.assertEQ(it, b.data[idx])
        ++idx;
    }
    
    idx = 0;
    for(let it of new B) {
        arktest.assertEQ(it, b.data[idx])
        ++idx;
    }

    return 0;
}
