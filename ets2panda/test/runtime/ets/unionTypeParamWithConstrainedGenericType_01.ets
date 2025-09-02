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
}

class B extends A {
}

function foo<T extends A>(a: T | string): T | string {
    if (a instanceof A) {
      const x: T = a as T;
      return x;
    }
    else {
      const y: string = a as string;
      return y;
    }
}
function main(){
  const a = new A();
  arktest.assertEQ(foo(a), a)
  const b = new B();
  arktest.assertEQ(foo(b), b)
  arktest.assertEQ(foo(""), "")
  arktest.assertEQ(foo("123"), "123")
}