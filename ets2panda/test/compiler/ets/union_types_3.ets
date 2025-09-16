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

function main() {
    let x0 : int | String | boolean = 2;
    arktest.assertEQ((x0 as int) * 2, 4, "Error! Must be 4");
    x0 = -2;
    arktest.assertEQ((x0 as int) * 3, -6, "Error! Must be -6");
    x0 = true
    arktest.assertEQ((x0 as boolean), true, "Error! Must be true");
    x0 = "STR1"
    arktest.assertTrue(((x0 as String) + "STR2").equals("STR1STR2"), "Error! Must be `STR1STR2`");
    let x1 : String | boolean = "AAA";
    if (x1 instanceof String) {
        arktest.assertTrue((x1 as String).equals("AAA"), "Error! Must be `AAA`");
    }
    let x2 : String | boolean | int | double = true;
    arktest.assertEQ((x2 as boolean), true, "Error! Must be `true`");
    let x3 : String | boolean | int | double = 3.14;
    // arktest.assertEQ((x3 as double), 3.14, "Error! Must be `3.14`" // #15576);
    arktest.assertEQ((x3 as Double), 3.14, "Error! Must be `3.14`");
}

