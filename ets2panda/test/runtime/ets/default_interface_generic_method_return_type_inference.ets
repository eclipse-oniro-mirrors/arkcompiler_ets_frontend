/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

interface I {
    meth<T>(p: T) { return 'i' }
}

class A implements I {
    meth<T>(p: T) { return 'a' }
}

let i: I = new A()
let a: A = new A()

function main() {
    arktest.assertEQ(i.meth(1), 'a')
    arktest.assertEQ(i.meth<string>(''), 'a')
    arktest.assertEQ(a.meth(1), 'a')
    arktest.assertEQ(a.meth<string>(''), 'a')
}
