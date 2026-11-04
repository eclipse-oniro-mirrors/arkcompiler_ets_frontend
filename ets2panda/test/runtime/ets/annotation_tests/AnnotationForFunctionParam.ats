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

// Annotation declaration:
@Retention("SOURCE")
@interface Anno {
    name: string = "Jim"
    id: int = 1
}

@Retention("SOURCE")
@interface Anno2 {
    name: string
}

function foo1(@Anno x : int, @Anno2({name: "name1"}) y : int) {
}
function foo2(@Anno() x : int, @Anno2({name: "name2"}) y : int) {
}
function foo3(@Anno({}) x : int, @Anno2({name: "name3"}) y : int) {
}
function foo4(@Anno({}) x: int, @Anno2({name: "name3"}) ...y : int[]) {
}
function foo5(@Anno({}) x: int, @Anno2({name: "name3"}) y ?: int) {
}
function foo6(@Anno() @Anno2({name: "name4"}) x : int, @Anno @Anno2({name: "name4"}) y : int) {
}

function main() {
    foo1(1, 2)
    foo2(3, 4)
    foo3(5, 6)
    foo4(7, 8)
    foo5(9, 10)
    foo6(11, 12)
}
