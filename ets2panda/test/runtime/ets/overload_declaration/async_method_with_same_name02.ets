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
    static async getData1() {
        return "I'm a static method1 with name 'getData'";
    }
    static async getData2(s: string) {
        return s;
    }
    static async overload getData{ getData1, getData2 }
}

function main() {
    let res1: string = await A.getData()
    let res2: string = await A.getData("I'm a static method2 with name 'getData'")
    arktest.assertEQ(res1, "I'm a static method1 with name 'getData'")
    arktest.assertEQ(res2, "I'm a static method2 with name 'getData'")
}
