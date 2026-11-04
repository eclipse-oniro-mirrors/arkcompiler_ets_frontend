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

class A {
    field_: string = "";
}

get field(this: A): string { return this.field_ }
set field(this: A, v: string) { this.field_ = v }

function foo(this: A, s: string): string {
    this.field = s;
    return this.field + " accessor with receiver in extension function"
}

function boo(a: A, s: string): string {
    a.field = s;
    return a.field + " accessor with receiver in normal function"
}

function main(): void {
    let a = new A();
    arktest.assertEQ(a.foo("call"), "call accessor with receiver in extension function")
    arktest.assertEQ(foo(a, "call"), "call accessor with receiver in extension function")
    arktest.assertEQ(boo(a, "call"), "call accessor with receiver in normal function")
}