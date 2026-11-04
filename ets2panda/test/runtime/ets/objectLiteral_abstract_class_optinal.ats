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

abstract class C{
    field1?:number;
    field2?:string;
}

function main(){
    let x:C = {field1:1000,field2:"some str"}
    arktest.assertEQ(x.field1, 1000)
    arktest.assertEQ(x.field2, "some str")

    let x2:C = {field2:"some str"}
    arktest.assertEQ(x2.field1, undefined)
    arktest.assertEQ(x2.field2, "some str")

    let x3:C = {}
    arktest.assertEQ(x3.field1, undefined)
    arktest.assertEQ(x3.field2, undefined)
}
