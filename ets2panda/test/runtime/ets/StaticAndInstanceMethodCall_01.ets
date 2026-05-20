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

class A {

 foo(): string {
    return "instance";
  }

 static foo(): string {
    return "static";
  }
}

class B {
  private a1_: A = new A();
  a2: A = new A();

  get a(): A {
    return this.a1_;
  }

  bar(): A {
    return this.a1_;
  }
}   

class C {
  private b_: B = new B();

  get b(): B {
    return this.b_;
  }
}


function main() {
  let c = new C();

  let x: A = c.b.a;
  arktest.assertEQ(x.foo(), c.b.a.foo());

  x = new B().bar();
  arktest.assertEQ(x.foo(), new B().bar().foo());

  x = new B().a2;
  arktest.assertEQ(x.foo(), new B().a2.foo());
} 

