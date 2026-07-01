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
    static overload $_instantiate { $_instantiateInt, $_instantiateString }

    static $_instantiateInt(factory: ()=>Base, p: int): string {
        return "int:" + p
    }

    static $_instantiateString(factory: ()=>Base, p: string): string {
        return "string:" + p
    }
}

class Derived extends Base {
}

function main(): void {
    arktest.assertEQ(Base(10), "int:10")
    arktest.assertEQ(Base("Hello"), "string:Hello")
    arktest.assertEQ(Derived(20), "int:20")
    arktest.assertEQ(Derived("World"), "string:World")
}
