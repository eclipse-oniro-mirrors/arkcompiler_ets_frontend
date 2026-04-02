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

// Test cases for bridge methods with interface implementation

interface I {
    foo(num: number): number
    foo2(num: number, a: number): number
}

class A implements I {
    foo(num: number): number {
        return num
    }
    foo2(num: number, a: number): number {
        return a
    }
}

class B extends A {
    override foo(num: number): number {
        return num + 1
    }
    override foo2(num: number, a: number): number {
        return a
    }
}

function bar1(obj: I): number {
    return obj.foo(1)
}

function bar2(obj: I): number {
    return obj.foo2(1, 2)
}

function main() {
    let ts = new arktest.ArkTestsuite("override_interface_method")

    ts.addTest("bar1 through interface", () => {
        let b = new B()
        arktest.assertEQ(bar1(b), 2)
    })

    ts.addTest("bar2 through interface", () => {
        let b = new B()
        arktest.assertEQ(bar2(b), 2)
    })

    return ts.run()
}
