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

function main(): void {
    let preA : a;

    type a = Double;

    let a : a;

    a = 1.1;
}

function foo(){

    switch (1) {
        case 0:     
            type dsa = Double;
            let dsa : dsa;
            break
        case 1:
            let asd: asd;
            break
        default :
            type asd = number;
    }

    {
        let asd : asd;
        type asd = number;
        type dsa = Double;
        let dsa : dsa;
    }

    while (true) {
        let asd : asd;
        type asd = Object;
        type dsa = Double;
        let dsa : dsa;
        break;
    }

    for (let k: int = 0; k < 10; k++) {
        let asd : asd;
        type asd = Double;

        type dsa = Double;
        let dsa : dsa;
    }

    try {
        let asd : asd;
        type asd = Double;

        type dsa = Double;
        let dsa : dsa;
    } catch (e){
        let asd : asd;
        type asd = Double;

        type dsa = Double;
        let dsa : dsa;
    }

}

 
/* @@? 17:16 Error Semantic error ESE0371: Cannot find type 'a'. */
/* @@? 19:5 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
/* @@? 21:13 Error Semantic error ESE0371: Cannot find type 'a'. */
/* @@? 30:13 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
/* @@? 31:23 Error Semantic error ESE0371: Cannot find type 'dsa'. */
/* @@? 34:22 Error Semantic error ESE0371: Cannot find type 'asd'. */
/* @@? 37:13 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
/* @@? 41:19 Error Semantic error ESE0371: Cannot find type 'asd'. */
/* @@? 42:9 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
/* @@? 43:9 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
/* @@? 44:19 Error Semantic error ESE0371: Cannot find type 'dsa'. */
/* @@? 48:19 Error Semantic error ESE0371: Cannot find type 'asd'. */
/* @@? 49:9 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
/* @@? 50:9 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
/* @@? 51:19 Error Semantic error ESE0371: Cannot find type 'dsa'. */
/* @@? 56:19 Error Semantic error ESE0371: Cannot find type 'asd'. */
/* @@? 57:9 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
/* @@? 59:9 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
/* @@? 60:19 Error Semantic error ESE0371: Cannot find type 'dsa'. */
/* @@? 64:19 Error Semantic error ESE0371: Cannot find type 'asd'. */
/* @@? 65:9 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
/* @@? 67:9 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
/* @@? 68:19 Error Semantic error ESE0371: Cannot find type 'dsa'. */
/* @@? 70:19 Error Semantic error ESE0371: Cannot find type 'asd'. */
/* @@? 71:9 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
/* @@? 73:9 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
/* @@? 74:19 Error Semantic error ESE0371: Cannot find type 'dsa'. */
