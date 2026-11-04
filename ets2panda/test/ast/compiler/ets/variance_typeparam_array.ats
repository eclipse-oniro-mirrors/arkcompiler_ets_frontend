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

class A<in T1,out T2,T3>{
    field1:/* @@ label1 */T1[];   //CTE
    field2:/* @@ label2 */T2[];   //CTE
    field3:T3[];
    readonly readonlyfield1:/* @@ label3 */T1[];   //CTE
    readonly readonlyfield2:/* @@ label4 */T2[];   //CTE
    readonly readonlyfield3:T3[];

    returnTypeFunc1():/* @@ label5 */T1[]{   //CTE
        return this.readonlyfield1;
    }

    returnTypeFunc2():/* @@ label6 */T2[]{   //CTE
        return this.readonlyfield2;
    }

    returnTypeFunc3():T3[]{
        return this.readonlyfield3;
    }

    paramsFunc1(a:/* @@ label7 */T1[]){}   //CTE
    paramsFunc2(a:/* @@ label8 */T2[]){}   //CTE
    paramsFunc3(a:T3[]){}
    paramsFunc4(a:/* @@ label9 */T1[], b:/* @@ label10 */T2[]){}   //CTE
    paramsFunc5(a:/* @@ label11 */T1[], b:T3[]){}   //CTE
    paramsFunc6(a:/* @@ label12 */T2[], b:T3[]){}   //CTE
    paramsFunc7(a:/* @@ label13 */T1[], b:/* @@ label14 */T2[], c:T3[]){}   //CTE
}


/* @@@ label1 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label2 Error Semantic error ESE0331: Type Parameter 'T2' is declared as 'out' but occurs in 'invariant' position. */
/* @@@ label3 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label4 Error Semantic error ESE0331: Type Parameter 'T2' is declared as 'out' but occurs in 'invariant' position. */
/* @@@ label5 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label6 Error Semantic error ESE0331: Type Parameter 'T2' is declared as 'out' but occurs in 'invariant' position. */
/* @@@ label7 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label8 Error Semantic error ESE0331: Type Parameter 'T2' is declared as 'out' but occurs in 'invariant' position. */
/* @@@ label9 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label10 Error Semantic error ESE0331: Type Parameter 'T2' is declared as 'out' but occurs in 'invariant' position. */
/* @@@ label11 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label12 Error Semantic error ESE0331: Type Parameter 'T2' is declared as 'out' but occurs in 'invariant' position. */
/* @@@ label13 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label14 Error Semantic error ESE0331: Type Parameter 'T2' is declared as 'out' but occurs in 'invariant' position. */
