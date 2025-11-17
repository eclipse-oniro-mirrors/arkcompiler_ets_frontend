/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

class C {
    v1: int = 0
    v2: string = ""
}

type U = C | Record<string, string>;

let d: U = /* @@ label1 */{"v2": "xxx"}
let e: U = /* @@ label2 */{"v5": 77}

/* @@@ label1 Error Semantic error ESE0061: Ambiguous reference to 'C|Record<String,String>' */
/* @@@ label2 Error Semantic error ESE0063: Target type for class composite needs to be an object type, found 'C|Record<String,String>' */
