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

function main() {
    let [a, b, c] = [1, 2, 3];
    [a, b, c] = ["abc", 20, 30];

    let aa = 10, bb = 20, cc = "cc";
    [aa, bb, cc] = [1, 2, 3];
}

/* @@? 18:18 Error Semantic error ESE0318: Type '"abc"' cannot be assigned to type 'Int' */
/* @@? 21:27 Error Semantic error ESE0318: Type 'Int' cannot be assigned to type 'String' */
