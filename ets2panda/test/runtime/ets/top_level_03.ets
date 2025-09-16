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

let x = 1;

struct A {
  private x: int;

  constructor() { 
    this.x = x; 
  };

  getx(): int { return this.x }
}

let a = new A();

x = 10;

struct B {
  private x: int = ETSGLOBAL.x;

  getx(): int { return this.x }
}

let b = new B;

x = 20;

function main() : void 
{
  arktest.assertEQ(a.getx(), 1)
  arktest.assertEQ(b.getx(), 10)
  arktest.assertEQ(x, 20)
}
