/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
    data: T;
}
function foo<T>(this: B<T>, d: T): this {
    this.data = d;
    return this
}

function main() {
    let b = new B<Int>();
    arktest.assertEQ(b.foo(1).data, 1)
    arktest.assertEQ(foo(b, 2).data, 2)
    arktest.assertEQ(b.foo<Int>(3).data, 3)
    arktest.assertEQ(foo<Int>(b, 4).data, 4)
}