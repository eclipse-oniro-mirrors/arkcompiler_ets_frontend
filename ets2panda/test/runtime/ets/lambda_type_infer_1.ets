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

interface A {
    num: Int;
}

interface test {
    t: A[]
}

interface B {
    unsorted: A[];
}

class C {
    static foo(...arg: B[]): void { 
        arktest.assertEQ(arg[0].unsorted[0].num, 2);
    };
}

function main() {
    const tests: test[] = [{ t: [{ num: 1 }] }];
    C.foo(...tests.map((test): B => ({
        unsorted: [...test.t.map((m): A => ({num:2}))],
    })));
}
