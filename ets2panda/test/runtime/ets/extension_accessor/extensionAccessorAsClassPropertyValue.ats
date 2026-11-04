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
    idx_: Int = 1;
}

get idx(this: A): Int {
    return this.idx_;
}

class B {
    a: A = new A();
    data_: Int = this.a.idx;
    data_2: Int = new A().idx;
}

function main() {
    arktest.assertEQ(new B().data_, 1)
    arktest.assertEQ(new B().data_2, 1)
}
