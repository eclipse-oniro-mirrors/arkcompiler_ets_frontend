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
    x: string | undefined
}

interface I {
    x: string
}

function foo<T extends A>(a: T | I): T | I {
    if (a instanceof A) {
      const x: T = a as T;
      return x;
    }
    else {
      const y: I = a as I;
      return y;
    }
}

function main(){
  const a = new A();
  arktest.assertEQ(foo(a), a)
  const i: I = {x: "a"};
  arktest.assertEQ(foo(i), i)
}
