/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

type custom_tuple_type = [string, int];

class A {foo(){}}
class B extends A {bar(){}}

function main(): void {
    let a_tup: [[int, number], [custom_tuple_type], [A, B], custom_tuple_type[]] = [[2, 3], [["a", 6]], [new B(), new B()], [["b", 8], ["c", 10], ["d", 12]]];
    let my_str: String = a_tup[1][0][0];
    let my_str_2: String = a_tup[3][0][0];
    a_tup[2][0].foo();
    a_tup[2][1].bar();
}
