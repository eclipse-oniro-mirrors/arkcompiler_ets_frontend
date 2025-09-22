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

namespace NS {
    export get name() {return '1'}
}

namespace NS1 {
    export set name(a:string) {}
}

set nameTop(a:string) {}

get nameTop() { return '1'; }

function main() {
    let nsName : int = NS.name;
    let nsName1 : int = 10;
    nsName1 = NS.name;
    NS1.name = 1;

    let topName : int = nameTop;
    let topName1 : int = 10;
    topName1 = nameTop;
    nameTop = 1;
}

/* @@? 29:24 Error Semantic error ESE0318: Type 'String' cannot be assigned to type 'Int' */
/* @@? 31:15 Error Semantic error ESE0318: Type 'String' cannot be assigned to type 'Int' */
/* @@? 32:16 Error Semantic error ESE0318: Type 'Int' cannot be assigned to type 'String' */
/* @@? 34:25 Error Semantic error ESE0318: Type 'String' cannot be assigned to type 'Int' */
/* @@? 36:16 Error Semantic error ESE0318: Type 'String' cannot be assigned to type 'Int' */
/* @@? 37:15 Error Semantic error ESE0318: Type 'Int' cannot be assigned to type 'String' */
