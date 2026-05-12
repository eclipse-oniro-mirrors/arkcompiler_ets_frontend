/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

/*
 * Test: template literal interpolation with void return method call.
 * When a method returns undefined-typed field value, template literal
 * should produce "undefined" string, not crash or produce "null".
 */

class A {
    x = undefined
    get() {
        return this.x
    }
}

function main() {
    let a = new A()

    let s1: string = `${a.get()}`
    arktest.assertEQ(s1, "undefined")

    let s2: string = `${undefined}`
    arktest.assertEQ(s1, s2)

    let s3: string = `${a.get()}_${a.get()}`
    arktest.assertEQ(s3, "undefined_undefined")

    let s4: string = `42_${a.get()}`
    arktest.assertEQ(s4, "42_undefined")
}
