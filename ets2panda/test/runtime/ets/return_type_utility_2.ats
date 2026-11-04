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

function foo<R extends Function>(a0: ReturnType<R>): ReturnType<R> { return a0; }

class A { check: boolean = true }

function main(): void {
    let a: A|Int = foo<(()=>A) | (()=>Int)>(new A());
    let b: A|Int = foo<(()=>A) | (()=>Int)>(2);

    let c: ReturnType<ReturnType<()=>(()=>A)>> = new A();
    arktest.assertTrue(c.check)

    let d: ReturnType<ReturnType<()=>(()=>A)> | ReturnType<()=>(()=>Int)>> = new A();
    arktest.assertTrue(d.check)
    let e: ReturnType<ReturnType<()=>(()=>A)> | ReturnType<()=>(()=>Int)>> = 3;
    arktest.assertEQ(e, 3);
}
