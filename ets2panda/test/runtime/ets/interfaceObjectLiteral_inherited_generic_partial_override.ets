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

interface PartialBase {
    m1(p: short): string;
    m2<T>(p: T): string;
    overload foo { m1, m2 }
}

interface PartialDerived extends PartialBase {
    m2<T>(p: T): string { return "GEN" }
}

function main(): void {
    let obj: PartialDerived = {
        m1(p: short): string { return "custom" }
    }

    arktest.assertEQ(obj.m1(1 as short), "custom")
    arktest.assertEQ(obj.foo(1 as short), "custom")
    arktest.assertEQ(obj.foo(1 as int), "GEN")
}
