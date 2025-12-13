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

let keyof = 0;
keyof = 1

class A {
  keyof = 1
}

class B {
  readonly keyof = 1
}

interface I{
  test(keyof: String): String;
}

class C implements I{
  test(keyof: String): String{
    return keyof;
  }
}

abstract class AD {
  readonly keyof = 1
  abstract test(keyof: String): String;
}

class D extends AD{
  test(keyof: String): String{
    return keyof;
  }
}

function main(): void{
  arktest.assertEQ(keyof, 1)
  let ca = new A()
  ca.keyof = 2
  arktest.assertEQ(ca.keyof, 2)
  let cb = new B()
  arktest.assertEQ(cb.keyof, 1)
  let cc = new C()
  arktest.assertEQ(cc.test("test"), "test")
  let cd = new D()
  arktest.assertEQ(cd.test("test"), "test")
  arktest.assertEQ(cd.keyof, 1)
}
