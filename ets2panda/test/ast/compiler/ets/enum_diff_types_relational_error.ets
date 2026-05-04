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

enum E1 { A, B }
enum E2 { C, D }

function main() {
    let x = E1.A > E2.C;
    let y = E1.A < E2.C;
}

/* @@? 20:13 Error Semantic error ESE0114: Bad operand type, the types of the operands must be numeric, same enumeration, or boolean type. */
/* @@? 21:13 Error Semantic error ESE0114: Bad operand type, the types of the operands must be numeric, same enumeration, or boolean type. */

