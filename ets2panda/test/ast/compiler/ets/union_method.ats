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

class A {
    shorting(v: string): void {}
    ordinary(a: int, b: int, c: int): void {}
    range(a: int, b: int, c: int, d: int, e: int, f: int): void {}
}

class B {
    shorting(v: string): void {}
    ordinary(a: int, b: int, c: int): void {}
    range(a: int, b: int, c: int, d: int, e: int, f: int): void {}
}

function shorting(x: A|B) {
    x.shorting("123")
}

function ordinary(x: A|B) {
    x.ordinary(1, 2, 3)
}

function range(x: A|B) {
    x.range(1, 2, 3, 4, 5, 6)
}
