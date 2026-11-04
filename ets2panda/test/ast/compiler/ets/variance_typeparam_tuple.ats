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
    field1:/* @@ label1 */[number,T1,string];   //CTE
    field2:/* @@ label2 */[number,T2,string];   //CTE
    field3:[number,T3,string];
    readonly readonlyfield1:/* @@ label3 */[number,T1,string];   //CTE
    readonly readonlyfield2:/* @@ label4 */[number,T2,string];  //CTE
    readonly readonlyfield3:[number,T3,string];

    returnTypeFunc1():/* @@ label5 */[number,T1,string]{   //CTE
        return this.readonlyfield1;
    }

    returnTypeFunc2():/* @@ label6 */[number,T2,string]{   //CTE
        return this.readonlyfield2;
    }

    returnTypeFunc3():[number,T3,string]{
        return this.readonlyfield3;
    }

    paramsFunc1(a:/* @@ label7 */[number,T1,string]){}  //CTE
    paramsFunc2(a:/* @@ label8 */[number,T2,string]){}   //CTE
    paramsFunc3(a:[number,T3,string]){}
}


/* @@@ label1 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label2 Error Semantic error ESE0331: Type Parameter 'T2' is declared as 'out' but occurs in 'invariant' position. */
/* @@@ label3 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label4 Error Semantic error ESE0331: Type Parameter 'T2' is declared as 'out' but occurs in 'invariant' position. */
/* @@@ label5 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label6 Error Semantic error ESE0331: Type Parameter 'T2' is declared as 'out' but occurs in 'invariant' position. */
/* @@@ label7 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label8 Error Semantic error ESE0331: Type Parameter 'T2' is declared as 'out' but occurs in 'invariant' position. */