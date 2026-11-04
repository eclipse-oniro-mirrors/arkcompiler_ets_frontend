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

type I2v = (i: int) => void
type T1 = (lambda: (arg: int) => int, arg: int) => int
type T2 = (c: int) => int

const F1: I2v = (counter: int) => {
    let funcWithLambda: T1 = (lambda: (arg: int) => int, arg: int) => { return lambda(arg) }
    let it: T2 = (c: int): int => {return c}
    // TODO(gogabr): As of 2023.10.10, loops at runtime (#13652/#I83OEH)
    while (counter > 0) counter = funcWithLambda(it, counter)
}

function main() {
    F1(44)
    return 0;
}
