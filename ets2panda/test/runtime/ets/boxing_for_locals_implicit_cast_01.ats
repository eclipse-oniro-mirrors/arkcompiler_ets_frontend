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

function foo(): Int {
    return 42
}

function test_capture_and_assign_number(): void {
    let v: number;
    let executed = false;
    let f = () => {
        v = foo()
        executed = true
    };
    f();
    arktest.assertTrue(executed)
    arktest.assertEQ(v, 42.0)
}

function test_capture_and_assign_int(): void {
    let v: Int;
    let executed = false;
    let f = () => {
        v = foo()
        executed = true
    };
    f();
    arktest.assertTrue(executed)
    arktest.assertEQ(v, 42)
}

function test_capture_and_assign_double(): void {
    let d: Double;
    let executed = false;
    let f = () => {
        d = 3.14
        executed = true
    };
    f();
    arktest.assertTrue(executed)
    arktest.assertEQ(d, 3.14)
}

function main(): void {
    test_capture_and_assign_number()
    test_capture_and_assign_int()
    test_capture_and_assign_double()
}