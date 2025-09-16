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

class B<T> {
    value: T;
    constructor(value: T) {
        this.value = value;
    }
}

type FB<T> = (this: B<T>, x: T) => T;
type FBS = (this: B<string>, x: string) => string;

function bar(this: B<string>, x: string): string {
    return this.value + x;
}

let f1: FB<string> = bar;
let f2: FBS = bar;
let b = new B<string>("Hello!");
arktest.assertEQ(b.f1("world"), "Hello!world")
arktest.assertEQ(f1(b, "world"), "Hello!world")
arktest.assertEQ(b.f2("world"), "Hello!world")
arktest.assertEQ(f2(b, "world"), "Hello!world")

f1 = bar;
f2 = bar;
arktest.assertEQ(b.f1("world"), "Hello!world")
arktest.assertEQ(f1(b, "world"), "Hello!world")
arktest.assertEQ(b.f2("world"), "Hello!world")
arktest.assertEQ(f2(b, "world"), "Hello!world")

f1 = f2;
arktest.assertEQ(b.f1("world"), "Hello!world")
arktest.assertEQ(f1(b, "world"), "Hello!world")

f2 = f1;
arktest.assertEQ(b.f2("world"), "Hello!world")
arktest.assertEQ(f2(b, "world"), "Hello!world")
