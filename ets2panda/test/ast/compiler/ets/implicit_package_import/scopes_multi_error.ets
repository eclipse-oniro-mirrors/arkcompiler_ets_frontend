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
    return;
}

function main(i : string[]): void {
    return;
}

function main(i : string[], g : int): void {
    return;
}

export default function TestFunc(): void {}
export default function TestFuncToo(): void {}


/* @@? 20:10 Error Semantic error ESE0373: Main overload is not enabled */
/* @@? 20:15 Error Semantic error ESE0135: Only 'FixedArray<string>' type argument is allowed. */
/* @@? 24:10 Error Semantic error ESE0373: Main overload is not enabled */
/* @@? 24:10 Error Semantic error ESE0133: 0 or 1 argument are allowed */
/* @@? 29:16 Error Semantic error ESE0374: Only one default export is allowed in a module */