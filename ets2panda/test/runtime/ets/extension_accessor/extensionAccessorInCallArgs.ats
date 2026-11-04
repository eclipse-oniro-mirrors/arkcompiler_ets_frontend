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
    name_: string = "Bob";
    age_: int = 22;
    count_:Int = 11;
}

get name(this: A): string {
    return this.name_;
}

get age(this: A): int {
    return this.age_;
}

get count(this: A): Int {
    return this.count_;
}

function foo(n: string, a:int, c: Int) {
    arktest.assertEQ(n, "Bob")
    arktest.assertEQ(a, 22)
    arktest.assertEQ(c, 11)
}

let a = new A();
foo(a.name, a.age, a.count);
foo(new A().name, new A().age, new A().count);
