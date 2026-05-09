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

function test_capture_and_read_number(): void {
    let v: number = 100.5;
    let result: number = 0.0;
    let f = () => {
        result = v
    };
    f();
    arktest.assertEQ(result, 100.5)
}

function test_capture_and_read_int(): void {
    let v: Int = 55;
    let result: Int = 0;
    let f = () => {
        result = v
    };
    f();
    arktest.assertEQ(result, 55)
}

function test_capture_and_read_double(): void {
    let v: Double = 2.718;
    let result: Double = 0.0;
    let f = () => {
        result = v
    };
    f();
    arktest.assertEQ(result, 2.718)
}

function test_capture_read_and_write(): void {
    let v: number = 10.0;
    let result: number = 0.0;
    let f = () => {
        result = v
        v = 20.0
    };
    f();
    arktest.assertEQ(result, 10.0)
    arktest.assertEQ(v, 20.0)
}

function main(): void {
    test_capture_and_read_number()
    test_capture_and_read_int()
    test_capture_and_read_double()
    test_capture_read_and_write()
}