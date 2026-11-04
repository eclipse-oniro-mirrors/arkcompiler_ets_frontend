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
 * Test virtual method calls through base type
 */

class Base {
    foo(p: string): string {
        return "Base"
    }
}

class Derived extends Base {
    foo(p: string): string {
        return "Derived"
    }
}

class BaseMulti {
    method(a: string, b: int): string {
        return "BaseMulti"
    }
}

class DerivedMulti extends BaseMulti {
    method(a: string, b: int): string {
        return "DerivedMulti"
    }
}

function main() {
    let ts = new arktest.ArkTestsuite("override_object_primitive")

    ts.addTest("Virtual call through base type - single param", () => {
        let base = new Derived() as Base
        arktest.assertEQ(base.foo("test"), "Derived")
    })

    ts.addTest("Virtual call through base type - multi param", () => {
        let base = new DerivedMulti() as BaseMulti
        arktest.assertEQ(base.method("a", 1), "DerivedMulti")
    })

    return ts.run()
}
