/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
    namespace ns {
        dfdfsfdf
    }
    namespace hehe {

    }
    time: number = foo();
    private uri: string;

    private routers() {
        B.getInstance().navigation(this);
    }

    constructor(uri: string) {
        this.uri = uri;
    }
}

class B {
    namespace ns {

    }ddddddd
    time: number = foo();
    private uri: string;

    private routerssdsds() {
        B.getInstance().navigation(this);
    }

    constructor(uri: string) {
        this.uri = uri;
    }
}

/* @@? 17:5 Error SyntaxError: Namespace is allowed only at the top level or inside a namespace. */
/* @@? 18:9 Error TypeError: Unresolved reference dfdfsfdf */
/* @@? 20:5 Error SyntaxError: Namespace is allowed only at the top level or inside a namespace. */
/* @@? 23:20 Error TypeError: Unresolved reference foo */
/* @@? 27:11 Error TypeError: Property 'getInstance' does not exist on type 'B' */
/* @@? 36:5 Error SyntaxError: Namespace is allowed only at the top level or inside a namespace. */
/* @@? 38:13 Error SyntaxError: Field type annotation expected. */
/* @@? 39:20 Error TypeError: This expression is not callable. */
/* @@? 43:11 Error TypeError: Property 'getInstance' does not exist on type 'B' */
