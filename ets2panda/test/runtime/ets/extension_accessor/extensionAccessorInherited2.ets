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
    xstate: string = "";
}

class B extends A {}

get x(this: B): string {
    return this.xstate + "B"
}
set x(this: B, v: string) { 
    this.xstate = "B" + v
}

get x(this: A): string { 
    return this.xstate + "A";
}
set x(this: A, v: string) { this.xstate = "A" + v }

function main() {
    let b = new B();
    b.x = "call";
    arktest.assertEQ(b.x, "BcallB")
    arktest.assertEQ(b.xstate, "Bcall")

    let a = new A();
    a.x = "call";
    arktest.assertEQ(a.x, "AcallA")
    arktest.assertEQ(a.xstate, "Acall")
}