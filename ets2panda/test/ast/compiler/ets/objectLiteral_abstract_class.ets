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
    field1:number = 100;
    private field2:string = "some str";
}

abstract class C2{
    constructor(x:number){}
    field1:number = 100;
    field2:string = "some str";
}

abstract class C3{
    private constructor(){}
    field1:number = 100;
    field2:string = "some str";
}

function main(){
    let x:C = {field1:1000, /* @@ label */field3:2000};
    let x2:C = {field1:1000, /* @@ label2 */field2:2000};
    let x3:C = {field1: /* @@ label3 */"some str"};
    let x4:C2 = /* @@ label4 */{field1:1000, field2:"some str"};
    let x5:C3 = /* @@ label5 */{field1:1000, field2:"some str"};
}


/* @@@ label Error Semantic error ESE0065: type C has no property named field3 */
/* @@@ label2 Error Semantic error ESE0293: Property field2 is not visible here. */
/* @@@ label3 Error Semantic error ESE0319: Type '"some str"' is not compatible with type 'Double' at property 'field1' */
/* @@@ label4 Error Semantic error ESE0291: Type C2 has no parameterless constructor. Initialization with literals is not supported if the type has no parameterless constructor. Declare the parameterless constructor explicitly or remove parametered constructors! */
/* @@@ label5 Error Semantic error ESE0139: Signature constructor(): void is not visible here. */