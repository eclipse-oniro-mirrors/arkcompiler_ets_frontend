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
    xstate: int = 0;
}

get x(this: A): int { return this.xstate }
set x(this: A, v: int) { this.xstate = v }

class B extends A {}

function main() {
    let b = new B();
    b.x = 123;
    arktest.assertEQ(b.xstate, 123)
    arktest.assertEQ(b.x, 123)
}