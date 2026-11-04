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

class A {
  protected name_: char;

  public constructor() {
    this.name_ = c'A';
  }

  public name(): char {
    return this.name_;
  }

  public a(): char {
    return c'A';
  }
}

final class B extends A {
  public constructor() {
    this.name_ = c'B';
  }

  public b(): char {
    return c'B';
  }
}

function reference_reference_test(): void {
  let B_ = new B();
  arktest.assertEQ(B_.name(), c'B')
  arktest.assertEQ(B_.b(), c'B')

  let B_A = B_ as A;
  arktest.assertEQ(B_A.name(), c'B')
  arktest.assertEQ(B_A.a(), c'A')

  let A_B = B_A as B;
  arktest.assertEQ(B_.name(), c'B')
  arktest.assertEQ(B_.b(), c'B')
}

function array_test(): void {
  let Bs: FixedArray<B> = [new B(), new B(), new B()];

  let B_As: FixedArray<A> = Bs as FixedArray<A>;
  for (let i: int = 0; i < B_As.length; i++) {
    arktest.assertEQ(B_As[i].name(), c'B')
    arktest.assertEQ(B_As[i].a(), c'A')
  }

  let Object_: Object = B_As as Object;

  let Object_Bs: FixedArray<B> = Object_ as FixedArray<B>;
  for (let i: int = 0; i < Object_Bs.length; i++) {
    arktest.assertEQ(Object_Bs[i].name(), c'B')
    arktest.assertEQ(Object_Bs[i].b(), c'B')
  }
}

function multi_array_test(): void {
  let Bs: FixedArray<FixedArray<B>> = [[new B()], [new B()], [new B()]];

  let B_As: FixedArray<FixedArray<A>> = Bs as FixedArray<FixedArray<A>>;
  for (let i: int = 0; i < B_As.length; i++) {
    for (let j: int = 0; j < B_As[i].length; j++) {
      arktest.assertEQ(B_As[i][j].name(), c'B')
      arktest.assertEQ(B_As[i][j].a(), c'A')
    }
  }

  let Object_: Object = B_As as Object;

  let Object_Bs: FixedArray<FixedArray<B>> = Object_ as FixedArray<FixedArray<B>>;
  for (let i: int = 0; i < Object_Bs.length; i++) {
    for (let j: int = 0; j < Object_Bs[i].length; j++) {
      arktest.assertEQ(Object_Bs[i][j].name(), c'B')
      arktest.assertEQ(Object_Bs[i][j].b(), c'B')
    }
  }
}

function primitive_reference_test(): void {
  let int_: int = 42;
  let Int_ = int_ as Int;
  arktest.assertTrue(Int_ instanceof Int)
  arktest.assertEQ(Int_.toInt(), 42)
  arktest.assertEQ(Int_.add(1) as int, 43)
}

function main(): void {
  reference_reference_test();
  array_test();
  multi_array_test();
  primitive_reference_test();
}
