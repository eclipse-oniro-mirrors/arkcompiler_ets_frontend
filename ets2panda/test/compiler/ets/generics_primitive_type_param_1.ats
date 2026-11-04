/*
 * Copyright (c) 2021-2026 Huawei Device Co., Ltd.
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

class A<T> {
    public value: T = undefined as T;
    public prim_value: int = 5;
}

function foo(a0: int): void {}

function test(): void {
    let a_int: A<int> = new A<int>();
    a_int.value = 2;
    foo(a_int.value);
    let v1 = a_int.value;
    let v2: int = a_int.value;
    let v3: boolean = v1 == 2;
    a_int.value = a_int.prim_value;

    let a_double: A<double> = new A<double>();
    a_double.value = 1.5;
    let v4 = a_double.value - 1.5;

    let a_boolean: A<boolean> = new A<boolean>();
    a_boolean.value = false;
    let v5 = a_boolean.value == false;
}
