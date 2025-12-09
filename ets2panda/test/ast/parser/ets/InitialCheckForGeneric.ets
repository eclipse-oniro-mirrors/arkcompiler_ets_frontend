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

function foo<U, V>() {
    let u:U // CTE
    let v:V // CTE

    console.log("What ? :-)", /* @@ label */u, /* @@ label1 */v)
}



function main(): void {
    foo<string, string>()
}


/* @@@ label Error Semantic error ESE0375: Variable 'u' is used before being assigned. */
/* @@@ label1 Error Semantic error ESE0375: Variable 'v' is used before being assigned. */