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

// Same wrong-context diagnostic, but in a binary expression without a conditional wrapper:
// BinaryExpression::CleanCheckInformation clears the subtree, and the erroneous
// identifier must be re-checked on a later overload attempt with the diagnostic kept.

function main() {
    console.log(`${(typeof 2) === string}`)
}

/* @@? 21:35 Error Semantic error ESE0144: Type name 'string' used in the wrong context */
