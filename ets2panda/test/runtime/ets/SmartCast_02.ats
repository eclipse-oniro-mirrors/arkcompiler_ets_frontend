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
  bar(): string {
    return "Class A";
  }
}

class B extends A {}

class C extends B {
  bar(): string {
    return "Class C";
  }
}

function foo(c: Int|String|A|null|undefined): void {
  if (c instanceof String)  {
    arktest.assertEQ(c.length, 11)
  } else if (c instanceof C) {
    arktest.assertEQ(c.bar(), "Class C")
  } else if (c instanceof Int) {
    arktest.assertEQ(c * c, 49)
  } else if (c === undefined) {
     arktest.assertEQ(c, undefined)
  } else {
     arktest.assertEQ(c, null)
  }
}

function main(): void {
  foo("Test string");
  foo(new Int(7));
  foo(new C());
  foo(null);
  foo(undefined);
}
