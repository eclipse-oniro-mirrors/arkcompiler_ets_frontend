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

class A<T> {
    fld: T
    constructor(p: T) {
        this.fld = p
    }
}

class B {
    fld: string = 'b'
}

function main() {
    let caught_counter = 0

    try {
        foo(new A<number>(11) as Object)
    } catch (error: ClassCastError) {
        caught_counter++
    }

    arktest.assertEQ(caught_counter, 1)
}

function foo(obj: Object) {
    return (obj as A<string>|B).fld
}
