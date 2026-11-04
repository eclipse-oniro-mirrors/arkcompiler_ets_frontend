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

class Base1 {
    foo(p: Array<string>): void {}
}
class Derived1 extends Base1 {
    foo(p: Array<number>): void {}
}

class Base2 {
    foo(p: [int, long]): void {}
}
class Derived2 extends Base2 {
    foo(p: [float, double]): void {}
}

class Base3 {
    foo(p: 'a' | 'b' | 'c'): void {}
}
class Derived3 extends Base3 {
    foo(p: 'd' | 'e' | 'f'): void {}
}

class Base4 {
    foo(p: string | undefined): void {}
}
class Derived4 extends Base4 {
    foo(p: Object): void {}
}

interface Base5 {
    foo(p: Array<string>): void {}
}
interface Derived5 extends Base5 {
    foo(p: Array<number>): void {}
}

interface Base6 {
    foo(p: Array<string>): void {}
}
class Derived6 implements Base6 {
    foo(p: Array<number>): void {}
}

function main(): void {}
/* @@? 20:8 Error Semantic error ESE0131: Function foo with this assembly signature already declared. */
/* @@? 27:8 Error Semantic error ESE0131: Function foo with this assembly signature already declared. */
/* @@? 34:8 Error Semantic error ESE0131: Function foo with this assembly signature already declared. */
/* @@? 41:8 Error Semantic error ESE1029298: Member foo in Derived4 cannot override foo in Base4 because a parameter type of the overridden method contains a nullish value that is not a subtype of the corresponding parameter type of the overriding method, while the non-nullish part of that parameter type is non-empty and is a subtype of it. */
/* @@? 48:5 Error Semantic error ESE0131: Function foo with this assembly signature already declared. */
/* @@? 55:8 Error Semantic error ESE0131: Function foo with this assembly signature already declared. */
