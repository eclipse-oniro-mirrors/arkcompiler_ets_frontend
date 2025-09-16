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

class A { name = "Bob" }
let a = new A();
let show = (this: A): string => {
    return "Hi," + this.name;
}

function foo(this: A): string {
    return "Hello," + this.name;
}

let show2: (this: A) => string;
show2 = show;
arktest.assertEQ(show2(a), "Hi,Bob")
arktest.assertEQ(a.show2(), "Hi,Bob" )
arktest.assertEQ(show2(new A()), "Hi,Bob")
arktest.assertEQ(new A().show2(), "Hi,Bob" )
show2 = foo;
arktest.assertEQ(show2(a), "Hello,Bob")
arktest.assertEQ(a.show2(), "Hello,Bob" )
arktest.assertEQ(show2(new A()), "Hello,Bob")
arktest.assertEQ(new A().show2(), "Hello,Bob" )
