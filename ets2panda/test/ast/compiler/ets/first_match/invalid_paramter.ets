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

class A {}
class B {}

class Test {
    foo1(a?: A,) {
        console.log("invoke1")
    }
    foo2(a?: A, b?: B) {
        console.log("invoke2")
    }

    overload foo111{ foo1, foo2, foo3, foo4 }
}

function main() {
    let a: Test = new Test();
    a.foo111(new A(), new A())
    a.foo111(new A(), new A(), new A())
}


/* @@? 27:34 Error Semantic error ESE0384: overloaded name must refer to an accessible method. */
/* @@? 27:40 Error Semantic error ESE0384: overloaded name must refer to an accessible method. */