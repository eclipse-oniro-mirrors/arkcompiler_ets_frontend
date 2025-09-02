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

class CIterator implements Iterator<string> {

  private ind_: int = 0;
  private base_: C;

  constructor (base: C) {
    this.base_ = base;
  }

  override next(): IteratorResult<string> {
    return {
      done: this.base_[this.ind_].equals("none"),
      value: this.base_[this.ind_++]
    }
  }
}

class C {
  private y: string[] = ["one", "two", "three"];

  $_get(ind: int) : string {
    if (ind >= 0 && ind < 3) return this.y[ind];
    return "none";
  }

  $_iterator(): CIterator {
    return new CIterator(this);
  }

  $_set(ind: string, val: string): void  {
    if (ind == "1") this.y[0] = val;
    if (ind == "2") this.y[1] = val;
    if (ind == "3") this.y[2] = val;
  }

}

function foo(c: C): void {
  c["2"] = 'ДВА!';
}

function main(): void {

  let c = new C();
  let i: int = 0;

  for (let it of c) {
    it += ": in for";

    if (i == 0) {
       arktest.assertEQ(it, "one: in for");
    } else if (i == 1) {
       arktest.assertEQ(it, "two: in for")
    } else if (i == 2) {
       arktest.assertEQ(it, "three: in for")
    } else {
       arktest.assertTrue(false);
    }

    ++i;
  }

  foo(c);

  i = 0;
  let it: string = "init";
  for (it of c) {
    it += ": after foo";

    if (i == 0) {
       arktest.assertEQ(it, "one: after foo");
    } else if (i == 1) {
       arktest.assertEQ(it, "ДВА!: after foo")
    } else if (i == 2) {
       arktest.assertEQ(it, "three: after foo")
    } else {
       arktest.assertTrue(false);
    }

    ++i;
  }
}
