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

class Index {
    private button : B [] = [];

    async aboutToAppear() {
        this.buttonsOptions.push({
            text: $r(value: T),
            adRequestParams: {}
        });
    }
}

interface B {
    text: ResourceStr;
    adRequestParams: advertising.adRequestParams;
}


/* @@? 20:14 Error Semantic error ESE0087: Property 'buttonsOptions' does not exist on type 'Index' */
/* @@? 21:27 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 21:27 Error Syntax error ESY0016: Unexpected token. */
/* @@? 21:30 Error Syntax error ESY0228: Unexpected token, expected ':'. */
/* @@? 21:31 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 22:13 Error Syntax error ESY0016: Unexpected token. */
/* @@? 22:30 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 23:10 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 25:1 Error Syntax error ESY0227: Unexpected token '}'. */
/* @@? 28:11 Error Semantic error ESE0371: Cannot find type 'ResourceStr'. */
/* @@? 29:22 Error Semantic error ESE0371: Cannot find type 'advertising'. */
/* @@? 29:34 Error Semantic error ESE0070: 'adRequestParams' type does not exist. */