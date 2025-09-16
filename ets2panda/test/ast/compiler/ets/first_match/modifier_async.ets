/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
    async fooNumber(a: number): Promise<void> {}
    fooString(a: string): void {}
    overload foo {/* @@ label1 */fooNumber, fooString }
}

class Base2 {
    async fooNumber(a: number): Promise<void> {}
    fooString(a: string): void {}
    async overload foo {fooNumber, /* @@ label2 */fooString }
}

/* @@@ label1 Error TypeError: Overload alias and overloaded method name must have exactly the same modifiers (static, async). */
/* @@@ label2 Error TypeError: Overload alias and overloaded method name must have exactly the same modifiers (static, async). */
