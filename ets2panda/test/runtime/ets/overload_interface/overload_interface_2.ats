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

interface A<T> {
    foo (p: T): string
}
interface B<R> {
    foo (p: R): string
}
interface C extends A<int>, B<double> {
}
class Impl implements C {
    foo (p: int): string {
        return p.toString();
    }
    foo (p: double): string {
        return p.toString();
    }
}
function main() {
    let i: Impl = new Impl();
    arktest.assertEQ(i.foo(1), "1");
    arktest.assertEQ(i.foo(1.2), "1.2");
}
