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

class A<in T1,out T2,T3>{
    field1:/* @@ label1 */(a:/* @@ label2 */T1)=>T2;   //CTE
    field2:/* @@ label3 */(a:/* @@ label4 */T2)=>T1;   //CTE
    field3:(a:T3)=>T3;
    readonly readonlyfield1:(a:T1)=>T2;
    readonly readonlyfield2:/* @@ label5 */(a:/* @@ label6 */T2)=>T1;   //CTE
    readonly readonlyfield3:(a:T3)=>T3;

    returnTypeFunc1():(a:T1)=>T2{
        return this.readonlyfield1;
    }

    returnTypeFunc2():/* @@ label7 */(a:/* @@ label8 */T2)=>T1{   //CTE
        return this.readonlyfield2;
    }

    returnTypeFunc3():(a:T3)=>T3{
        return this.readonlyfield3;
    }

    paramsFunc1(a:/* @@ label9 */(a:/* @@ label10 */T1)=>T2){}   //CTE
    paramsFunc2(a:(a:T2)=>T1){}
    paramsFunc3(a:(a:T3)=>T3){}
    paramsFunc4(a:(a:(a:T1)=>T2)=>T1){}
    paramsFunc5(a:/* @@ label11*/(a:(a:T1)=>T1)=>T1){}   //CTE
    paramsFunc6(a:/* @@ label12*/(a:(a:T2)=>T2)=>T1){}   //CTE
    paramsFunc7(a:(a:(a:(a:T2)=>T1)=>T2)=>T1){}
}

/* @@@ label1 Error Semantic error ESE0331: Type Parameter 'T2' is declared as 'out' but occurs in 'invariant' position. */
/* @@@ label1 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label3 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label3 Error Semantic error ESE0331: Type Parameter 'T2' is declared as 'out' but occurs in 'invariant' position. */
/* @@@ label5 Error Semantic error ESE0332: Type Parameter 'T1' is declared as 'in' but occurs in 'out' position. */
/* @@@ label5 Error Semantic error ESE0333: Type Parameter 'T2' is declared as 'out' but occurs in 'in' position. */
/* @@@ label7 Error Semantic error ESE0332: Type Parameter 'T1' is declared as 'in' but occurs in 'out' position. */
/* @@@ label7 Error Semantic error ESE0333: Type Parameter 'T2' is declared as 'out' but occurs in 'in' position. */
/* @@@ label9 Error Semantic error ESE0333: Type Parameter 'T2' is declared as 'out' but occurs in 'in' position. */
/* @@@ label9 Error Semantic error ESE0332: Type Parameter 'T1' is declared as 'in' but occurs in 'out' position. */
/* @@@ label11 Error Semantic error ESE0332: Type Parameter 'T1' is declared as 'in' but occurs in 'out' position. */
/* @@@ label12 Error Semantic error ESE0333: Type Parameter 'T2' is declared as 'out' but occurs in 'in' position. */
