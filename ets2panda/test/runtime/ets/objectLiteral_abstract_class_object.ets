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

interface A{
    field1:number;
}

class B{
    field1:number = 0;
}

abstract class C{
    field1:number = 0;
}

abstract class D{
    field1?:number;
    field2?:string;
    field3:A = {field1:0};
    field4:B = {field1:0};
    field5:C = {field1:0};
}

function main(){
    let x:D = {field1:1000,field2:"some str",field3:{field1:10}}
    arktest.assertEQ(x.field1, 1000)
    arktest.assertEQ(x.field2, "some str")
    arktest.assertEQ(x.field3.field1, 10)

    let x2:B = new B();
    x2.field1 = 10;
    let x3:D = {field4:x2}
    arktest.assertEQ(x3.field4.field1, 10)

    let x4:B = {field1:10}
    let x5:D = {field4:x4}
    arktest.assertEQ(x5.field4.field1, 10)

    let x6:D = {field5:{field1:10}}
    arktest.assertEQ(x6.field5.field1, 10)
}
