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

type UT = `Test escape characters \a\\\'\" \' \b \f \n \r \t \v \\ \x7F \u005c
              Test escape characters` | "X";

function main(){
    let x  = `Test escape characters \a\\\'\" \' \b \f \n \r \t \v \\ \x7F \u005c
              Test escape characters`;
    let x2:UT = `Test escape characters \a\\\'\" \' \b \f \n \r \t \v \\ \x7F \u005c
              Test escape characters`
    arktest.assertEQ(x, x2)
    arktest.assertEQ(x2, `Test escape characters \a\\\'\" \' \b \f \n \r \t \v \\ \x7F \u005c
              Test escape characters`)
    arktest.assertEQ(x2, "Test escape characters \a\\\'\" \' \b \f \n \r \t \v \\ \x7F \u005c\n              Test escape characters")
}
