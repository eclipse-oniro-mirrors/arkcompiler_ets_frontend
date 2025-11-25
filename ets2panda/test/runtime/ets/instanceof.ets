/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

function bar() {}

function foo(): () => void {
    return bar;
}

function main(): void {
  let obj = new Boolean();
  let nobj: Object|null = null;

  arktest.assertTrue(null instanceof null)
  arktest.assertTrue(nobj instanceof null)
  arktest.assertTrue(!(obj instanceof null))

  let arr: FixedArray<int> = [1, 2, 3];

  arktest.assertTrue(arr instanceof Object)
  arktest.assertTrue(!(arr instanceof Long))
  arktest.assertTrue(obj instanceof Object)
  arktest.assertTrue(obj instanceof Boolean)
  arktest.assertTrue(!(obj instanceof Long))

  let intArr: FixedArray<int> = [1, 2, 3];

  arktest.assertTrue(intArr instanceof FixedArray<int>)
  arktest.assertTrue(intArr instanceof Object)
  arktest.assertTrue(!(intArr instanceof Long))
  arktest.assertTrue((intArr instanceof FixedArray<Int>))
  arktest.assertTrue(!(intArr instanceof Int))

  let integerArr: FixedArray<Int> = new FixedArray<Int>(10);

  arktest.assertTrue(integerArr instanceof FixedArray<Int>)
  arktest.assertTrue(integerArr instanceof Object)
  arktest.assertTrue(!(intArr instanceof FixedArray<Number>))
  arktest.assertTrue((integerArr instanceof FixedArray<int>))
  arktest.assertTrue(!(integerArr instanceof Int))

  let integerArrArr: FixedArray<FixedArray<int>> = [[10], [20]];

  arktest.assertTrue(integerArrArr instanceof FixedArray<FixedArray<int>>)
  arktest.assertTrue(integerArrArr instanceof Object)
  arktest.assertTrue(!(integerArrArr instanceof FixedArray<Int>))
  arktest.assertTrue(!(integerArrArr instanceof Int))
  arktest.assertTrue(!(integerArrArr instanceof FixedArray<FixedArray<Long>>))

  let f: () => void = foo();

  arktest.assertTrue(f instanceof (() => void))

  arktest.assertTrue(bar instanceof (() => void))

  return;
}
