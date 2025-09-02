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
  field: int = 5

  constructor(arg: boolean) {
  }

  constructor(arg?: int) {
    if (arg == undefined) {
      this.field += 20
    } else {
      this.field += arg;
      return; // Return statement is necessary to reproduce stack overflow and invalid constructor generation issue.
    }
  }
}

class B {
  field: int = 5;

  constructor(arg: boolean) {
  }

  constructor(arg: int) {
    this.field += arg;
    return; // Return statement is necessary to reproduce stack overflow issue.
  }

  constructor() {
    this(15);
    this.field += 30;
  }
}

function main(): void {
  arktest.assertEQ(new A(true).field, 5);  // field = 5
  arktest.assertEQ(new A(10).field, 15);   // field = 5 + 10
  arktest.assertEQ(new A().field, 25);     // field = 5 + 20

  arktest.assertEQ(new B(true).field, 5);  // field = 5
  arktest.assertEQ(new B(20).field, 25);   // field = 5 + 20
  arktest.assertEQ(new B().field, 50);     // field = 5 + 15 + 30
}
