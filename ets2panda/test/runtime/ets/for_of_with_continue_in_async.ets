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

/*---
desc: >-
  Compiler crashes due to empty finally block in async function
  Please refer the issue: #34322
---*/

async function foo() {
    let rec = new Record<string, string>();
    arktest.assertEQ(rec.size, 0);
    let num_iters = 0;
    for (const entries of rec.entries()) {
        num_iters++;
        if (!entries) {
            continue;
        }
    }
    arktest.assertEQ(num_iters, 0);
}

function main() {
    foo();
}
