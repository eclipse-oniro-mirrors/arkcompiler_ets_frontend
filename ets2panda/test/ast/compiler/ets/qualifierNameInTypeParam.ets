/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

export namespace NS1 {
    export interface I{
        a:string
        b:boolean
        array: number[]
    }
}

function foo(value:Object) {
    return value instanceof Array
}
function main(){
    let array = Array<NS1.I>();
    arktest.assertEQ(foo(array), true)
    let a: NS1.I = { a: "test", b: true, array: [1, 2, 3] }
    array.push(a);
    let firstElement = array[0] as NS1.I;
    arktest.assertEQ(firstElement.a, "test");
    arktest.assertEQ(firstElement.b, true);
    arktest.assertEQ(firstElement.array.length, 3);
}
