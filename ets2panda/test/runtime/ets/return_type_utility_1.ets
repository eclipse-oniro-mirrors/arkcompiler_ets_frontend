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

type MyString = ReturnType<()=>string>
type anAny = ReturnType<Function>
type aNever = ReturnType<never>

function foo1<P extends Function, R = ReturnType<P>>(a0: R): R { return a0; }
function foo2<P extends Function>(a0: ReturnType<P>): P|undefined { return undefined; }

class A { check: boolean = true }

function main(): void {
    let a: A = foo1<()=>A>(new A());
    arktest.assertTrue(a.check);

    let b: (()=>A)|undefined = foo2<()=>A>(new A());

    let c = foo1(new A());
    arktest.assertTrue(c.check);

    let d: Function|undefined = foo2(new A());
}
