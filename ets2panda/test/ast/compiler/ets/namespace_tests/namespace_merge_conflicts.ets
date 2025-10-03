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
// single file
namespace A {
    function foo() { }
    function bar() { }

    export namespace C {
        function too() { }
    }
}

namespace B {  }

namespace A {
    function goo() { bar(); }
    function foo() {  } // CTE
}

namespace A.C {
    function moo() { too(); }
}

/* @@? 29:5 Error Semantic error ESE0130: Function foo is already declared. */