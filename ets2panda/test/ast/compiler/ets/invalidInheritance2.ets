/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
    a: int = 1;
}

class B extends A {
}

class C extends B {
    /* @@ label */class a {

    }
}


/* @@? 23:17 Error Semantic error ESE0216: Cannot inherit from class B, because class a is inherited with a different declaration type */
/* @@@ label Error Syntax error ESY0233: Unexpected token. A constructor, method, accessor, or property was expected. */