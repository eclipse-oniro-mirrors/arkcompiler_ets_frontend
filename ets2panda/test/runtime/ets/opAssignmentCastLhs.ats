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

class A {
    name: string = "a"
    count: int = 3
}

class B {
    value: A = new A()
    data: int[] = [1, 2, 3]
}

let holder = new B()
let object_calls = 0
let index_calls = 0

// Regression test: compound assignment must work when the lhs object is a cast expression.
function append_from_object(x: Object | null | undefined) {
    (x as A).name += "1"
}

function append_from_union(x: A | string | undefined) {
    (x as A).name += "1"
}

function multiply_from_object(x: Object | null | undefined) {
    (x as A).count *= 2
}

function add_to_array(x: Object) {
    (x as int[])[1] += 10
}

function get_object_value(): Object {
    object_calls++
    return holder.value
}

function get_array_value(): Object {
    object_calls++
    return holder.data
}

function get_index(): int {
    index_calls++
    return 1
}

function test_object_cast_lhs() {
    let x: Object = new A()
    append_from_object(x)
    arktest.assertEQ((x as A).name, "a1")
}

function test_union_cast_lhs() {
    let x: A | string | undefined = new A()
    append_from_union(x)
    arktest.assertEQ((x as A).name, "a1")
}

function test_numeric_property_cast_lhs() {
    let x: Object = new A()
    multiply_from_object(x)
    arktest.assertEQ((x as A).count, 6)
}

function test_object_cast_lhs_is_evaluated_once() {
    holder.value.name = "a"
    object_calls = 0
    ;(get_object_value() as A).name += "1"
    arktest.assertEQ(object_calls, 1)
    arktest.assertEQ(holder.value.name, "a1")
}

function test_element_cast_lhs() {
    let x: Object = [1, 2, 3]
    add_to_array(x)
    arktest.assertEQ((x as int[])[1], 12)
}

function test_element_cast_lhs_is_evaluated_once() {
    holder.data = [1, 2, 3]
    object_calls = 0
    index_calls = 0
    ;(get_array_value() as int[])[get_index()] += 10
    arktest.assertEQ(object_calls, 1)
    arktest.assertEQ(index_calls, 1)
    arktest.assertEQ(holder.data[1], 12)
}

function main() {
    test_object_cast_lhs()
    test_union_cast_lhs()
    test_numeric_property_cast_lhs()
    test_object_cast_lhs_is_evaluated_once()
    test_element_cast_lhs()
    test_element_cast_lhs_is_evaluated_once()
}
