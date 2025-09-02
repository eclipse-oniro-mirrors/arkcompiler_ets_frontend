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

const c1 = 1 == 6 ? true : false;
const c2 = c1;

enum TestEnum {
    One = c2 ? 1 : 0,
    Two = true ? 5 : 10
}

function main() {

    arktest.assertEQ(TestEnum.One.valueOf(), 0);
    arktest.assertEQ(TestEnum.Two.valueOf(), 5);
}
