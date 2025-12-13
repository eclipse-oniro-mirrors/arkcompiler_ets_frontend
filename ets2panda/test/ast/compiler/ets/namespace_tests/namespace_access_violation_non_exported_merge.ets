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
// Example of accessing non-exported members in merged namespaces

namespace A {
    export namespace B {
        export function foo() { return 123; }
    }
}

namespace A.B {
    function bar() { return 456; } // Error: 'bar()' is not exported
}

A.B.bar(); // Error: 'bar()' cannot be accessed

/* @@? 27:5 Error Semantic error ESE0206: 'bar' is not exported in 'B' */