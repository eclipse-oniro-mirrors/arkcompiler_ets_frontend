/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

interface A {
  name?: string;

  foo(): string;  
  
  foo(a: string): string;
}

interface B extends A {
  value: string;
  foo(a: string): string  {
    return a;
  }
}

interface C extends B {
  foo(): string {
    return this.name!;
  }
}


function main() {
  let b = {name: "name", value: "value", foo(): string { return this.name!; }} as B;
  let c = {name: "name", value: "value"} as C;

  arktest.assertEQ(b.foo(), c.foo());
  arktest.assertEQ(c.foo(c.value), b.foo(b.value));
}


