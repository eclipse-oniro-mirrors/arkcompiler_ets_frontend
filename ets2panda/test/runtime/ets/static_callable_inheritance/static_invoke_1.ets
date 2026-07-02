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
    static $_invoke(p: boolean): string {
        if (p) {
            throw new Error("invoke error")
        }
        return "B"
    }
}

class Derived extends Base {
    $_invoke(): Derived {
        return new Derived()
    }
}

function invokeFromDerived(p: boolean): string {
    return Derived(p)
}

function main(): void {
    let checkException = (_e: Error): boolean => { return true }
    arktest.expectThrow(() => {
        invokeFromDerived(true)
    }, checkException)
    arktest.assertEQ(invokeFromDerived(false), "B")
}
