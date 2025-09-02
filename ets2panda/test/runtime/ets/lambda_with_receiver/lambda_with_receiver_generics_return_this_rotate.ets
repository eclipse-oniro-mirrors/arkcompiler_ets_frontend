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
    x: T;
    constructor(value: T) {
        this.x = value;
    }
    setX(x: T): this { this.x = x; return this }
}

class D<T> extends B<T> {
    y: T;
    constructor(value: T) {
        super(value);
        this.y = value;
    }
    setY(y: T): this { this.y = y; return this }
}

type F<T> = (this: B<T>) => this;
let rotate:F<int> = (this: B<int>): this => { return this }
let rotate2 = (this: B<int>): this => { return this }
function main() {
    let d = new D<int>(1).setX(3).rotate().setY(33);
    arktest.assertEQ(d.x, 3)
    arktest.assertEQ(d.y, 33)
    let dd = new D<int>(1).setX(6).rotate2().setY(66);
    arktest.assertEQ(dd.x, 6)
    arktest.assertEQ(dd.y, 66)

    let b = new B<int>(1).setX(6).rotate().setX(66);
    arktest.assertEQ(b.x, 66)
    let bb = new B<int>(1).setX(6).rotate2().setX(66);
    arktest.assertEQ(bb.x, 66)
}
