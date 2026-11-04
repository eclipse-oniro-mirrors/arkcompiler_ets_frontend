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

type X = Array<Int>

function foo<T>(this: X, el: int): T {
	return this[el] as T
}

function bar<B>(this: X, el: int): B {
    if(el == 0 || el == 1 || el == 2) { // trying deeper scope
	    let x1: B = this.foo<B>(el) 
	    return x1
    }
    let x1: B = foo<B>(this, el)
    return x1
}


function main() {
    let x: X = [1, 2, 3]
    arktest.assertEQ(x.bar<int>(0), 1)
    arktest.assertEQ(x.bar<int>(1), 2)
    arktest.assertEQ(x.bar<int>(2), 3)
}