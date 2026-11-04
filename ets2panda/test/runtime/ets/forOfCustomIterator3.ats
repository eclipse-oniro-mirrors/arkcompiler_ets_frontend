/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

class A implements Iterable<string>{
    data: string[] = ['a', 'b', 'c'];
    $_iterator():Iterator<string> {
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
            return new IteratorResult<string>(true, undefined);
        }
        return new IteratorResult<string>(this.index >= this.base.data.length, this.base.data[this.index++]);
    }
}


function fooIterable(a:Iterable<string>){
  let res = "";
  for (let x of a) res += x;
  arktest.assertEQ(res, "abc")
}

function main(): void {
    fooIterable(new A())
}
