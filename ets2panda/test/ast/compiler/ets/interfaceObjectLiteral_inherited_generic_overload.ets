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

interface GoodBase {
    m1(p: short): string;
    m2<T>(p: T): string;
    m3(p: byte): string { return "byte"; }
    overload foo1 { m2, m3, m1 }
}

interface GoodDerived extends GoodBase {
    m1(p: short): string { return "short"; }
    m2<T>(p: T): string { return "GEN"; }
}

let ok: GoodDerived = {};

ok.foo1(1 as int);
ok.foo1<string>("ok");

/* @@? 34:1 Error Semantic error ESE0256: Interface has methods */
interface Bad {
    m1<T>(p: T): string;
    m2(p: int): string { return "int"; }
    overload foo { m2 }
}

let bad: Bad = {};

bad.foo(1 as int);
