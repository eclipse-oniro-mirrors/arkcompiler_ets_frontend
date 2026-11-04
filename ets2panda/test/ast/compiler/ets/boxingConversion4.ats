/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

function refInt(a: Int): void {}

function main() : void {
    let a: Short = new Short(3); // 3 is of type short in this context
    let b: short = 2;
    let c: short = /* @@ label1 */-b; // due to unary operator promotion, '-b' will be int, which is not assignable
    let d: short = /* @@ label2 */b | b; // due to binary operator promotion, 'b | b' will be int, which is not assignable
    refInt(b);
}


/* @@@ label1 Error Semantic error ESE0318: Type 'Int' cannot be assigned to type 'Short' */
/* @@@ label2 Error Semantic error ESE0318: Type 'Int' cannot be assigned to type 'Short' */