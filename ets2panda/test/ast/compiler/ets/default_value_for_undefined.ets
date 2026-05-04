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
desc: For type 'undefined' and all its supertypes the default value is 'undefined'
name: default_value_for_undefined
tags:
- compile-only
---*/


function main() {
    let s1: string | undefined = undefined;
    let s2: string | undefined;

    if (/* @@ label1 */s1 instanceof undefined) {
        arktest.assertEQ(s1, undefined);
    } else {
        arktest.assertEQ(s1, "");
    }

    if (/* @@ label2 */s2 instanceof undefined) {
        arktest.assertEQ(s2, undefined);
    } else {
        arktest.assertEQ(s2, "");
    }
}


/* @@@ label1 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
/* @@@ label2 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
