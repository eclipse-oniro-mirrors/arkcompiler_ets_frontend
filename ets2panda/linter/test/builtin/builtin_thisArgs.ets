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

Class MyClass {
  base: number;
  constructor(base: number) {
    this.base = base;
  }
  compare((value: number, index:number, arr: Array<number>)) {
    return value < this.base
  }
}

let arr: Array<number> = new Array<number>(1, 2, 3); //BuiltinNewCtor
let a = new MyClass(2);
let b = new MyClass(3);
arr.filter(a.compare, a);
arr.filter(a.compare);
arr.filter(a.compare, b);

type T = number | T[];
let arr: ReadonlyArray<T> = [[1.0, 2.0], 3.0, [4.0, 5.0]];
arr.flatMap(
  (value, index, array) => {
    return value;
  }
);