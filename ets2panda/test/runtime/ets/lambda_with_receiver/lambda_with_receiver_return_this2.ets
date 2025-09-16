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
    x: number;
    setX(x: number): this { this.x = x; return this }
}

class B extends A {
    y: number;
    setY(y: number): this { this.y = y; return this }
}

let foo = (this: A): this => {
    this.data_ += 1;
    return this;
}

let rotate = (this: A): this => { return this }

function main() {
    arktest.assertEQ(new B().foo().data_, 2)
    arktest.assertEQ(foo(new B()).data_, 2)
    arktest.assertEQ(new B().foo().foo().data_, 3)
    arktest.assertEQ(foo(new B()).foo().data_, 3)
    let b = new B();
    arktest.assertEQ(b.foo().data_, 2)
    arktest.assertEQ(foo(b).data_, 3)
    arktest.assertEQ(b.foo().foo().data_, 5)
    arktest.assertEQ(foo(b.foo()).foo().data_, 8)

    b = new B().setX(3).rotate().setY(33);
    arktest.assertEQ(b.x, 3)
    arktest.assertEQ(b.y, 33)
}
