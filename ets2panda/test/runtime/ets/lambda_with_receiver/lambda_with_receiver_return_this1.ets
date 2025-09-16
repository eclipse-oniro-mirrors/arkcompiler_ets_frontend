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
    data_:number = 1;
}

let foo = (this: A): this => {
    this.data_ += 1;
    return this;
}

function main() {
    arktest.assertEQ((new A().foo()).data_, 2)
    arktest.assertEQ((foo(new A())).data_, 2)
    arktest.assertEQ((new A().foo().foo()).data_, 3)
    arktest.assertEQ((foo(new A().foo()).data_), 3)
    let a = new A();
    arktest.assertEQ(a.foo().data_, 2)
    arktest.assertEQ(foo(a).data_, 3)
    arktest.assertEQ(a.foo().foo().data_, 5)
    arktest.assertEQ(foo(a.foo().foo()).data_, 8)
}
