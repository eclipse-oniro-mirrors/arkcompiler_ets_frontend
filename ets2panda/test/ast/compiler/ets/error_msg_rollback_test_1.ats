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

namespace NS {
    export get name(): string{ return "hello" }
    export set name(arg: string){ console.log("123") }
    get age():number{ return 100 }
    set age(age: number){ console.log("age") }

    export function bar() {
        console.log(NS.age)
        console.log('test' + (NS.age = 200))
    }
}

function foo1(a: string) {

}
function foo1(a: number) {

}

foo1(NS.age)
NS.bar()

/* @@? 22:5 Error Semantic error ESE71336: Exported function 'bar' requires type annotation. */
/* @@? 23:24 Error Semantic error ESE0206: 'age' is not exported in 'NS' */
/* @@? 24:34 Error Semantic error ESE0206: 'age' is not exported in 'NS' */
/* @@? 35:9 Error Semantic error ESE0206: 'age' is not exported in 'NS' */
