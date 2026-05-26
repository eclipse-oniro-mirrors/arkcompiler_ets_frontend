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

interface AliasBase {
    m1(p: short): string;
    m2<T>(p: T): string;
    m3(p: byte): string { return "byte"; }
    overload foo1 { m2, m3, m1 }
    overload foo2 { m2, m3, m1 }
}

interface AliasDerived extends AliasBase {
    m1(p: short): string { return "short"; }
    m2<T>(p: T): string { return "GEN"; }
    overload foo1 { m1, m2, m3 }
}

interface MethodBase {
    m1(p: short): string { return "short"; }
    m2(p: int): string;
    m3<T>(p: T): string;
    overload m1 { m2, m1 }
    overload m2 { m1, m3, m2 }
}

interface MethodDerived extends MethodBase {
    m2(p: int): string { return "int"; }
    m3<T>(p: T): string { return "GEN"; }
    overload m1 { m1, m2, m3 }
}

function main() {
    let aliasObj: AliasDerived = {};
    arktest.assertEQ(aliasObj.foo1(1 as byte), "short")
    arktest.assertEQ(aliasObj.foo1(1 as int), "GEN")
    arktest.assertEQ(aliasObj.foo1<short>(1 as short), "GEN")
    arktest.assertEQ(aliasObj.foo2(1 as byte), "GEN")
    arktest.assertEQ(aliasObj.foo2<string>("x"), "GEN")

    let methodObj: MethodDerived = {};
    arktest.assertEQ(methodObj.m1(1 as byte), "short")
    arktest.assertEQ(methodObj.m1(1 as int), "int")
    arktest.assertEQ(methodObj.m1<short>(1 as short), "GEN")
    arktest.assertEQ(methodObj.m2(1 as byte), "int")
    arktest.assertEQ(methodObj.m2<short>(1 as short), "GEN")
}
