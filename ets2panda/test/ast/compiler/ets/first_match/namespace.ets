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

namespace NS {
    function foo1(a: number) {
        console.log("invoke foo1");
    }

    function foo2(a: string) {
        console.log("invoke foo2");
    }

    export overload overloadfoo{ foo1, foo2 }

    export function foo3(a: number) {
        console.log("invoke foo1");
    }

    function foo4(a: string) {
        console.log("invoke foo2");
    }

    export overload overloadfoo2{ foo3, foo4 }

    export function foo5(a: number) {
        console.log("invoke foo1");
    }

    function foo6(a: string) {
        console.log("invoke foo2");
    }

    overload overloadfoo3{ foo5, foo6 }
}

function main(){
    NS.foo1(1);
    NS.foo2("abc");
    NS.foo3(1);
    NS.foo4("abc");

    NS.overloadfoo(1);
    NS.overloadfoo("abc");

    NS.overloadfoo2(1);
    NS.overloadfoo2("abc");

    NS.overloadfoo3(1);
    NS.overloadfoo3("abc");
}

/* @@? 49:8 Error TypeError: 'foo1' is not exported in 'NS' */
/* @@? 50:8 Error TypeError: 'foo2' is not exported in 'NS' */
/* @@? 52:8 Error TypeError: 'foo4' is not exported in 'NS' */
/* @@? 54:5 Error TypeError: 'foo1' is not exported in 'NS' */
/* @@? 55:5 Error TypeError: 'foo2' is not exported in 'NS' */
/* @@? 58:5 Error TypeError: 'foo4' is not exported in 'NS' */
/* @@? 60:8 Error TypeError: 'overloadfoo3' is not exported in 'NS' */
/* @@? 61:5 Error TypeError: 'foo6' is not exported in 'NS' */
/* @@? 61:8 Error TypeError: 'overloadfoo3' is not exported in 'NS' */
