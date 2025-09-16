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

class A{
    field1:int;
    field2:int;
    method1(){};
}

class B extends A{
    bfield1:int;
    bfield2:int;
    bmethod1(){};
}

function main():void{
    let x1:keyof A = "field1";
    let x2:keyof A = "field2";
    let x3:keyof A = "method1";
    let x4:keyof A = /* @@ label1 */"other field";
    let x5:keyof A = /* @@ label2 */"other method";
    let x6:keyof A = /* @@ label3 */100;

    let y1:keyof B = "bfield1";
    let y2:keyof B = "bfield2";
    let y3:keyof B = "bmethod1";
    let y4:keyof B = /* @@ label4 */"other field";
    let y5:keyof B = /* @@ label5 */"other method";
    let y6:keyof B = /* @@ label6 */100;
}

/* @@@ label1 Error TypeError: Type '"other field"' cannot be assigned to type '"method1"|"field1"|"field2"' */
/* @@@ label2 Error TypeError: Type '"other method"' cannot be assigned to type '"method1"|"field1"|"field2"' */
/* @@@ label3 Error TypeError: Type 'Int' cannot be assigned to type '"method1"|"field1"|"field2"' */
/* @@@ label4 Error TypeError: Type '"other field"' cannot be assigned to type '"bmethod1"|"bfield1"|"bfield2"|"method1"|"field1"|"field2"' */
/* @@@ label5 Error TypeError: Type '"other method"' cannot be assigned to type '"bmethod1"|"bfield1"|"bfield2"|"method1"|"field1"|"field2"' */
/* @@@ label6 Error TypeError: Type 'Int' cannot be assigned to type '"bmethod1"|"bfield1"|"bfield2"|"method1"|"field1"|"field2"' */
