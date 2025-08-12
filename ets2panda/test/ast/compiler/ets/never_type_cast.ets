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

function foo() {
    let input = true;
    if (input instanceof Boolean) {
        console.log("This is a test function");
    } else {
        let b = input as (string | number | null);
        console.log("This is a test function with b: " + b);
    }
}
foo();

/* @@? 21:17 Error TypeError: Cannot cast type 'Boolean' to 'String|Double|null' */
