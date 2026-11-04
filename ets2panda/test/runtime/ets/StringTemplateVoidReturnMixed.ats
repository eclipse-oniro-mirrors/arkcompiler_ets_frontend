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
 * Test: void return consistency between assignment and template literal.
 * Both assignment and template literal should handle void return
 * consistently, producing the same undefined value.
 */

class B {
    x = undefined
    y = 42
    getUndef() {
        return this.x
    }
    getNum(): int {
        return this.y
    }
}

function explicitVoid(): void {}

function main() {
    let b = new B()

    let s1: string = `${b.getNum()}`
    arktest.assertEQ(s1, "42")

    let s2: string = `${b.getUndef()}`
    arktest.assertEQ(s2, "undefined")

    let s3: string = `${b.getNum()}_${b.getUndef()}`
    arktest.assertEQ(s3, "42_undefined")

    let s4: string = `${b.getUndef()}|${b.getNum()}|${b.getUndef()}`
    arktest.assertEQ(s4, "undefined|42|undefined")

    arktest.assertEQ(s2.length, 9)

    let s5: string = `${explicitVoid()}`
    arktest.assertEQ(s5, "undefined")

    let s6: string = `${b.getNum()}_${explicitVoid()}`
    arktest.assertEQ(s6, "42_undefined")
}
