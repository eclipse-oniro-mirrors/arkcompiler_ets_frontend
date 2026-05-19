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

class A {
    value: number = 1;
}

function foo(this: A){
    return this.value;
}

let foo2 = (this: A) => {
    return this.foo();
}

let a: A = new A();
console.log(a.foo());
console.log(a.foo2());

/* @@? 30:1 Error Semantic error ESE0127: No matching call signature for log(...) */
/* @@? 30:15 Error Semantic error ESE0087: Property 'foo2' does not exist on type 'A' */
