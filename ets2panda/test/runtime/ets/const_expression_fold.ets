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

function main() {
    const a : int = 1;
    const b : long = 1;
    const c : long = b as long;
    const d : float = 1;
    const e : number = 1;
    arktest.assertEQ(Type.of(a) instanceof IntType, true);
    arktest.assertEQ(Type.of(b) instanceof LongType, true);
    arktest.assertEQ(Type.of(c) instanceof LongType, true);
    arktest.assertEQ(Type.of(d) instanceof FloatType, true);
    arktest.assertEQ(Type.of(e) instanceof DoubleType, true);
}