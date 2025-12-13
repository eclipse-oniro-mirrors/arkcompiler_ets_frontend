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
  prop_: number;

  constructor(p: number) {
    this.prop_ = p;
  }

  xxx(a: number, b: number, c?: number, d?: number) {
    return this.prop_ + a + b;
  }
}

function foo(ass: Array<A>) {
  let a = 0;
  let b = 1;
  for (let i = 0; i < ass.length; i++) {
    let an = ass[i];
    let ann = an.xxx(8, 9);
    arktest.assertTrue(ann == 8 + 9 + i);
  }
  let c = 3;
  let d = 4;
  let e = 5;
  let f = 6;
  let g = 7;
}

foo([new A(0), new A(1)]);