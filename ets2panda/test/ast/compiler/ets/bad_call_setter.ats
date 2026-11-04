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

interface I {
    set text(s: string)
}

class B implements I {
    text: string = ""
    s : string
    this.text = s;
}

function main() {
    let ins: I = new B();
    ins.text = "He";
}


/* @@? 23:5 Error Syntax error ESY0227: Unexpected token 'this'. */
/* @@? 23:9 Error Syntax error ESY0227: Unexpected token '.'. */
/* @@? 23:10 Error Semantic error ESE0351: Variable 'text' has already been declared. */
/* @@? 23:17 Error Semantic error ESE0145: Property 's' must be accessed through 'this' */