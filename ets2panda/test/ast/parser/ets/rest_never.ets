/*
* Copyright (c) 2025 Huawei Device Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at 
* 
* http://www.apache.org/licenses/LICENSE-2.0
* 
* Unless required by applicable low or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

function sum(...nums: number[]) {
}

function main() {
    sum(...null!)
}

/* @@? 20:5 Error Semantic error ESE0127: No matching call signature for sum(...) */
/* @@? 20:9 Error Semantic error ESE0049: Spread expression can be applied only to array or tuple type, but 'never' is provided */
/* @@? 20:12 Warning Warning W0014: Bad operand type, the operand of the non-nullish expression is 'null' or 'undefined'. */