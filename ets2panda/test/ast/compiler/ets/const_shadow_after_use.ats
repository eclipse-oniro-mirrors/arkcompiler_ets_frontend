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

let tag: string = "outer";

function constShadow(): void {
    console.log(tag);
    const tag: string = "inner";
}

function letShadow(): void {
    console.log(tag);
    let tag: string = "inner";
}

/* @@? 19:17 Error Semantic error ESE0365: Variable 'tag' is accessed before it's initialization. */
/* @@? 24:17 Error Semantic error ESE0365: Variable 'tag' is accessed before it's initialization. */
