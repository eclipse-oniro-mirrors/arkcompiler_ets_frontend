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


// Interface default method with 'never' return type that always throws
interface INever {
    foo(): never {
        throw new Error("always throws")
    }
}

// Interface default method with 'int' return type that always returns
interface IReturn {
    bar(): int {
        return 42
    }
}

class C implements INever, IReturn {}

function main() {
    let c = new C()
    arktest.expectError(() => { c.foo() }, new Error("always throws"))
    arktest.assertEQ(c.bar(), 42)
}
