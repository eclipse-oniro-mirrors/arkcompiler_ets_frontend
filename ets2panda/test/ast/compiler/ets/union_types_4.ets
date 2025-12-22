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
    num: int = 42;
    foo() {}
}

class B {}

function main() {
    let x : String | short | A = 5 as short;
    arktest.assertEQ((x as short) * 2, 10, "Error! Must be 10");
    x = "STRSTR";
    arktest.assertTrue((x as String).equals("STRSTR"), "Error! Must be `STRSTR`")
    x = new A();
    arktest.assertEQ((x as A).num, 42, "Error! Field of A must be `42`");
    x = /* @@ label */new B();  // CTE
}


/* @@@ label Error Semantic error ESE0318: Type 'B' cannot be assigned to type 'String|Short|A' */