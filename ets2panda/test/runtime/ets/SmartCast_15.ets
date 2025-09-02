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

// Issue #22779 - smart cast in return statement
function isEmpty(text?: string): boolean {
    return text === undefined || text === null || text.length === 0
}

// Some other examples
function foo(text?: string): boolean {
    let a: boolean = (text === undefined || text === null || text.length == 0) ? true : false
    let b: boolean = text === undefined || text === null || text.length == 0
    let c: boolean = !(text !== undefined) || text === null || text.length == 0
    let d: boolean = (text !== undefined && text !== null) ? text.length == 0 : true
    let e: boolean = (text != undefined && text != null) ? text.length == 0 : true
    return a && b && c && d && e
}

function main(): void {
    arktest.assertEQ(isEmpty(), true)
    arktest.assertEQ(isEmpty(""), true)
    arktest.assertEQ(isEmpty("a"), false)
    arktest.assertEQ(foo(), true)
}
