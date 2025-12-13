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
  a: string = 'a';
  foo(b: B): A {
    return b as A;
  }
}

class B extends A {
  b: string = 'b';
  foo(a: A): B {
    return new B();
  }
}

interface I {
  c1: A;
  c2: B;
  foo (f: B): string | number {
    return f.b;
  };
}

function main() {

  let c: I = {
    c1: { a: 'aa' },
    c2: { a: 'aaa', b: 'bb' },
    foo (f: A): string {
      return f.a;
    },
  };

  arktest.assertEQ(c.c1.a, 'aa');
  arktest.assertEQ(c.c2.a, 'aaa');
  arktest.assertEQ(c.c2.b, 'bb');
  arktest.assertEQ(c.foo(c.c2), 'aaa');
}
