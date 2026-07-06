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

class Base {
    m(): Any {
        return "base"
    }
}

class Derived extends Base {
    override m(): void {
    }
}

function invokeFromBase(base: Base): Any {
    return base.m()
}

function chooseBase(flag: boolean): Base {
    return flag ? new Derived() : new Base()
}

function invokeAll(values: Base[]): Any[] {
    let result: Any[] = []
    for (let value of values) {
        result.push(value.m())
    }
    return result
}

function main() {
    let ts = new arktest.ArkTestsuite("override_bridge_void_to_any_returns_undefined")

    ts.addTest("plain base instance keeps base return value", () => {
        let base = new Base()
        arktest.assertEQ(base.m(), "base")
    })

    ts.addTest("virtual call through base returns undefined", () => {
        let base: Base = new Derived()
        arktest.assertEQ(base.m(), undefined)
    })

    ts.addTest("helper base call returns undefined", () => {
        arktest.assertEQ(invokeFromBase(new Derived()), undefined)
    })

    ts.addTest("direct derived call still succeeds", () => {
        let derived = new Derived()
        derived.m()
    })

    ts.addTest("factory returning Base preserves undefined for derived", () => {
        arktest.assertEQ(chooseBase(true).m(), undefined)
    })

    ts.addTest("factory returning Base preserves original base value", () => {
        arktest.assertEQ(chooseBase(false).m(), "base")
    })

    ts.addTest("virtual dispatch in array of Base returns mixed values", () => {
        let values = invokeAll([new Base(), new Derived(), new Derived()])
        arktest.assertEQ(values[0], "base")
        arktest.assertEQ(values[1], undefined)
        arktest.assertEQ(values[2], undefined)
    })

    return ts.run()
}
