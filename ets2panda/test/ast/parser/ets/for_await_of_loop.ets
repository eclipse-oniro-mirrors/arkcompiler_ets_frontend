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

for await (let k: int /* @@ label1 */= 0; /* @@ label2 */k < d.length; k++) {
    this.$_set_unsafe(k + 1, d[k])
}


/* @@@ label1 Error Syntax error ESY0097: for-await-of loop variable declaration may not have an initializer. */
/* @@@ label2 Error Syntax error ESY0227: Unexpected token 'k'. */
/* @@? 16:62 Error Semantic error ESE0143: Unresolved reference d */
/* @@? 17:5 Error Semantic error ESE0202: 'this' cannot be referenced from a static context */
/* @@? 17:10 Error Semantic error ESE0087: Property '$_set_unsafe' does not exist on type 'ETSGLOBAL' */