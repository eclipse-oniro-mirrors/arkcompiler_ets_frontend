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

class Test1{
    constructor(a?:number,b?:number){}
    constructor con1(){}
    overload constructor{con1}
}

class Test2{
    constructor(a:number,b:number){}
    constructor con1(){}
    overload constructor{con1}
}

class Test3{
    constructor(a:number,...args:[string,number]){}
    constructor con1(){}
    overload constructor{con1}
}

class Test4{
    constructor(a?:number,b?:number){}
    constructor(a:string){}
    constructor con1(){}
    /* @@ label1 */constructor con1(){}
    /* @@ label2 */overload constructor{con1}
}

class Test5{
    constructor(a:number,...args:[string,number]){}
    constructor(a:string){}
    constructor con1(){}
    /* @@ label3 */overload constructor{con1}
}


/* @@@ label1 Error Semantic error ESE0130: Function con1 is already declared. */
/* @@@ label2 Error Semantic error ESE0389: The overloaded name 'constructor' can't refer to a function with overload signatures. */
/* @@@ label2 Error Semantic error ESE0389: The overloaded name 'con1' can't refer to a function with overload signatures. */
/* @@@ label3 Error Semantic error ESE0389: The overloaded name 'constructor' can't refer to a function with overload signatures. */