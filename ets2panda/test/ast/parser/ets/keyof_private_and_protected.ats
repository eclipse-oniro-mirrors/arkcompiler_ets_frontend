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
    field1:number = 10;
    foo(){}
    private field2:number = 20;
    private foo2(){}
    protected field3:number = 30;
    protected foo3(){}
}

class B extends A{
    bfield1:number = 10;
    bfoo(){}
    private bfield2:number = 20;
    private bfoo2(){}
    protected bfield3:number = 30;
    protected bfoo3(){}
}

function main():void{
    let c1:keyof B = "field1"
    let c2:keyof B = "foo"
    let c3:keyof B = /* @@ label1 */"field2"  
    let c4:keyof B = /* @@ label2 */"foo2" 
    let c5:keyof B = /* @@ label3 */"field3"  
    let c6:keyof B = /* @@ label4 */"foo3"  
    let c7:keyof B = "bfield1"
    let c8:keyof B = "bfoo"
    let c9:keyof B = /* @@ label5 */"bfield2"   
    let c10:keyof B = /* @@ label6 */"bfoo2" 
    let c11:keyof B = /* @@ label7 */"bfield3"  
    let c12:keyof B = /* @@ label8 */"bfoo3"   
}


/* @@@ label1 Error Semantic error ESE0318: Type '"field2"' cannot be assigned to type '"bfoo"|"bfield1"|"foo"|"field1"' */
/* @@@ label2 Error Semantic error ESE0318: Type '"foo2"' cannot be assigned to type '"bfoo"|"bfield1"|"foo"|"field1"' */
/* @@@ label3 Error Semantic error ESE0318: Type '"field3"' cannot be assigned to type '"bfoo"|"bfield1"|"foo"|"field1"' */
/* @@@ label4 Error Semantic error ESE0318: Type '"foo3"' cannot be assigned to type '"bfoo"|"bfield1"|"foo"|"field1"' */
/* @@@ label5 Error Semantic error ESE0318: Type '"bfield2"' cannot be assigned to type '"bfoo"|"bfield1"|"foo"|"field1"' */
/* @@@ label6 Error Semantic error ESE0318: Type '"bfoo2"' cannot be assigned to type '"bfoo"|"bfield1"|"foo"|"field1"' */
/* @@@ label7 Error Semantic error ESE0318: Type '"bfield3"' cannot be assigned to type '"bfoo"|"bfield1"|"foo"|"field1"' */
/* @@@ label8 Error Semantic error ESE0318: Type '"bfoo3"' cannot be assigned to type '"bfoo"|"bfield1"|"foo"|"field1"' */