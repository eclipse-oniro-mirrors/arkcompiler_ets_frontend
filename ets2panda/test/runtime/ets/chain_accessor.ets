/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

let _name = 111
get name(): int { return _name }
set name(a: int) { _name = a }

function main() {
    // For jit purpose
    _name = 11;
    let c = name = 10;
    arktest.assertEQ(c, 10);
    arktest.assertEQ(name, 10);
    arktest.assertEQ(_name, 10);
}
