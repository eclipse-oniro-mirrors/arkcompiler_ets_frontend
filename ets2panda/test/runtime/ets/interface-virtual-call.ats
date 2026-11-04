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

interface iface {
  foo(): int;
}

class A implements iface {
  public override foo(): int {
    return 10;
  }
}

class B extends A {
  public override foo(): int {
    return 20;
  }
}

function main() : void {
    let a : A = new A();
    let b : B = new B();

    arktest.assertEQ(a.foo(), 10)
    arktest.assertEQ((a as iface).foo(), 10)
    arktest.assertEQ(b.foo(), 20)
    arktest.assertEQ((b as iface).foo(), 20)
}
