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
    let v: FixedArray<number> | null = [] // smart cast to non-nullish type

    // member expression (non computed)
    let n = v!.length

    // call expression
    let s = v!.toString()
}

/* @@? 20:13 Warning Warning W0015: Operand of the ensure-not-nullish expression is always non-nullish. */
/* @@? 23:13 Warning Warning W0015: Operand of the ensure-not-nullish expression is always non-nullish. */
