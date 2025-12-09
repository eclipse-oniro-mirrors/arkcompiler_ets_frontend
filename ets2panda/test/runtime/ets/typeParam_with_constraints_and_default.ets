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

class Base {
    id: number = 0;
}

class Derived extends Base {
    name: string = "";
}
function foo<T extends R, R = T>(a: T, b: R):T {
  return a
}

function main() {
    let derived: Derived = { id: 2, name: "derived"};
    let base: Base = { id: 3 };
    arktest.assertEQ(foo(derived, base).id, 2);
}