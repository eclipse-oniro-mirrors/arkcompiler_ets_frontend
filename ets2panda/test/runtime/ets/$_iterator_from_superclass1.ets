/*
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

interface RoArray<T>{
    val(i:int) : T
    len() : int
}

class XIterator<T> implements Iterator<T>{
    index:int = 0
    data:RoArray<T>
    constructor(p:RoArray<T>){
        this.data = p
    }
    override next(): IteratorResult<T>{
        return {
            done: this.index >= this.data.len(),
            value: this.index >= this.data.len()?undefined : this.data.val(this.index++)
        }
    }
}

interface I {
  data: RoArray<string>
  $_iterator() { return new XIterator<string>(this.data) }
}

class D implements I {
  data: RoArray<string>
  constructor(p: RoArray<string>) { this.data = p }
}

class Base extends D {
  constructor(p: RoArray<string>) { super(p) }
}

class C extends Base implements RoArray<string> {
  arr: string[] = ['a','b','c']
  constructor() {
    super(this)
  }
  override val(i : int):string{
     return this.arr[i]
  }
  override len():int{
      return this.arr.length
  }
}

function main(): int {
  let c = new C()
  let res = ''
  for (let x of c) {
    res += x
  }
    arktest.assertEQ(res,'abc')
    if(res!='abc') return 1
    return 0
}