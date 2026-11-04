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

type my_int = Int
type my_type<T> = T;
type a_t<U> = A<U>;

class A<L>{}

function main(): void {
    let a: Int = new my_type<Int>(2);
    let b: A<Double> = new A<my_type<Double>>();
    let c: A<my_type<Long>> = new A<Long>();
    let d: A<my_type<Int>> = new A<my_type<Int>>();

    let e: a_t<Int> = d;
    let f: a_t<Double> = b;
    let g: a_t<my_type<Int>> = d;
    let h: a_t<my_type<my_int>> = new A<Int>();
}
