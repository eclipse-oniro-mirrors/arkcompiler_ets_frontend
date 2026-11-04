/**
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

// Regression guard for rm-duplicate-class: multiple top-level untyped lambdas
// must each lower to exactly one class and compose correctly at runtime.
let a = (x: int): int => x + 1
let b = (x: int): int => x * 2

function main() {
    arktest.assertEQ(a(b(3)), 7)
}
