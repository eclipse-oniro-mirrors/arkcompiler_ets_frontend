/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

package mypackage;

function ClassTypeTest(a0: C1){
    arktest.assertEQ(C1.stmemb, "c1_stmemb")
    arktest.assertEQ(a0.memb, 2)
    arktest.assertEQ(a0.bar(), 3)
    arktest.assertEQ(a0.baz(), 4)
}

function main(): void {
    ClassTypeTest(new C1());
    let locVar: mytype = new A();
    arktest.assertEQ(locVar.aMemb, 5)
    arktest.assertEQ(globVar1, 7)
    arktest.assertEQ(globVar2, 7)
}

let globVar2: number;
static {
    globVar2 = globVar1;
}

function func2(a0: number): void {
    arktest.assertEQ(a0, 6)
}
