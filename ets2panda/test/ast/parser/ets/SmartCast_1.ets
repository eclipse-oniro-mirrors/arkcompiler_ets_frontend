/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

function foo(a: boolean | undefined) : int {
    if (a != undefined) {
        if (a == false) {
            return 0;
        } else {
            return 1;
        }
    } else {
    //a is undefined
        if (/* @@ label */a == false) {
            return 0;
        } else {
            return 1;
        }
    }
}

function main() {
    foo(true);
    foo(false);
    foo(undefined);
}

/* @@@ label Error TypeError: Bad operand type, the types of the operands must be numeric, same enumeration, or boolean type.  */
