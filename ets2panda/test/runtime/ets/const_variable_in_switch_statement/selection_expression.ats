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
let d = 1
const a = (d == 1) ? 2 : 3;
arktest.assertEQ(a, 2);
const b = (d == 0) ? 4 : 5;
arktest.assertEQ(b, 5);
let c = 5
let e = 0
switch (c) {
    case a:
        e = a;
        break;
    case b:
        e = b;
        break;
    default:
        break;
}

arktest.assertNE(e, a);
arktest.assertEQ(e, b);