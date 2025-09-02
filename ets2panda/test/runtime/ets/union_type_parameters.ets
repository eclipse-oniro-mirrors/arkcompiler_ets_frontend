/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

interface A<T>{}
class B<T>{}
class C<T> implements A<T>{}
class D{}
function foo<T>(x: A<T>|B<T>) {
    if (x instanceof A) {
        return 41;
    } else if (x instanceof B) {
        return 42;
    }
    return -1;
}

function main() {
    arktest.assertEQ(foo<D>(new B<D>()), 42,  "Error! branch with return 42 must be executed")
    arktest.assertEQ(foo<D>(new C<D>()), 41,  "Error! branch with return 41 must be executed")
}
