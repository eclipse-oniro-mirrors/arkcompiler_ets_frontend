/*
 * Copyright (c) 2023 - 2024-2025 Huawei Device Co., Ltd.
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
  static static_name: char = c'A';
  name: char = c'A';

  get_name(): char {
    return this.name;
  }

  static get_static_name(): char {
    return A.static_name;
  }
}

class B extends A {
  static static_name: char = c'B';
  name: char = c'B';

  static get_static_name(): char {
    return B.static_name;
  }

  super_name(): char {
    return A.static_name;
  }
}

final class C extends B {
  static static_name: char = c'C';
  name: char = c'C';

  static get_static_name(): char {
    return C.static_name;
  }

  override super_name(): char {
    return B.static_name;
  }
}

function main() : void {
  {
    let a = new A();
    arktest.assertEQ(A.static_name, c'A')
    arktest.assertEQ(A.get_static_name(), c'A')
    arktest.assertEQ(a.name, c'A')
    arktest.assertEQ(a.get_name(), c'A')
  }

  {
    let b = new B();
    arktest.assertEQ(B.static_name, c'B')
    arktest.assertEQ(B.get_static_name(), c'B')
    arktest.assertEQ(b.name, c'B')
    arktest.assertEQ(b.get_name(), c'B')
    arktest.assertEQ(b.super_name(), c'A')
  }

  {
    let b_as_a: A = new B();
    arktest.assertEQ(b_as_a.name, c'B')
    arktest.assertEQ(b_as_a.get_name(), c'B')
  }

  {
    let c = new C();
    arktest.assertEQ(C.static_name, c'C')
    arktest.assertEQ(C.get_static_name(), c'C')
    arktest.assertEQ(c.name, c'C')
    arktest.assertEQ(c.get_name(), c'C')
    arktest.assertEQ(c.super_name(), c'B')
  }

  {
    let c_as_a: A = new C();
    arktest.assertEQ(c_as_a.name, c'C')
    arktest.assertEQ(c_as_a.get_name(), c'C')
  }

  {
    let c_as_b: B = new C();
    arktest.assertEQ(c_as_b.name, c'C')
    arktest.assertEQ(c_as_b.get_name(), c'C')
    arktest.assertEQ(c_as_b.super_name(), c'B')
  }

}
