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

class A{}

class Test {
    constructor(a: number) {
        this.testString = "invoke1"
    }
    constructor(a: string){
        this.testString = "invoke2"
    }
    constructor(a: A){
        this.testString = "invoke3"
    }

    testString = ""
}

class Test2 {
    constructor(a: string){
        this.testString = "invoke4"
    }
    constructor(a: A){
        this.testString = "invoke5"
    }

    testString = ""
}

function main() {
    let var1 = new Test(10);
    let var2 = new Test("123");
    let var3 = new Test(new A());
    arktest.assertEQ(var1.testString, "invoke1")
    arktest.assertEQ(var2.testString, "invoke2")
    arktest.assertEQ(var3.testString, "invoke3")

    let var4 = new Test2("123");
    let var5 = new Test2(new A())
    arktest.assertEQ(var4.testString, "invoke4")
    arktest.assertEQ(var5.testString, "invoke5")
}
