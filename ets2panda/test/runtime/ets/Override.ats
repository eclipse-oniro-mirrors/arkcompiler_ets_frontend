/**
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http: //www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

interface I {
  self(): I;
  fn(): Object;
  fn2(): Object;
  fn3(): Object;
  fn4(): Object;
  fn5(): FixedArray<Object>;
}

class A implements I {
  public x: int = 42;
  override self(): A { return this; }
  override fn(): String { return ""; }
  override fn2(): FixedArray<Object> { return [new Object()]; }
  override fn3(): FixedArray<int> { return [0]; }
  override fn4(): FixedArray<int>[] { return [[0]]; }
  override fn5(): FixedArray<String> { return [""]; }
}


function main(): void {
  let a = new A();
  arktest.assertEQ(a.self().x, 42)
  arktest.assertEQ(a.fn(), "")
  arktest.assertEQ(a.fn2().length, 1)
  arktest.assertEQ(a.fn3()[0], 0)
  arktest.assertEQ(a.fn4()[0][0], 0)
  arktest.assertEQ(a.fn5()[0], "")

  // let i: I = a;
  // arktest.assertTrue(i.self() instanceof A)
  // arktest.assertEQ((i.fn() as String), "")
  // arktest.assertEQ((i.fn2() as FixedArray<Object>)[0], new Object()
  // arktest.assertEQ((i.fn3() as FixedArray<int>)[0], 0)
  // arktest.assertEQ((i.fn4() as FixedArray<String>)[0], "")
}


