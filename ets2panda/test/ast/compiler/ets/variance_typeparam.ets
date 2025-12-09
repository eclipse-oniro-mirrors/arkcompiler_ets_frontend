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
    field1:/* @@ label1 */T1;   //CTE
    field2:/* @@ label2 */T2;   //CTE
    field3:T3;
    readonly readonlyfield1:/* @@ label3 */T1;   //CTE
    readonly readonlyfield2:T2;
    readonly readonlyfield3:T3;

    returnTypeFunc1():/* @@ label4 */T1{   //CTE
        return this.readonlyfield1;
    }

    returnTypeFunc2():T2{
        return this.readonlyfield2;
    }

    returnTypeFunc3():T3{
        return this.readonlyfield3;
    }

    paramsFunc1(a:T1){}
    paramsFunc2(a:/* @@ label5 */T2){}   //CTE
    paramsFunc3(a:T3){}
    paramsFunc4(a:T1, b:/* @@ label6 */T2){}   //CTE
    paramsFunc5(a:T1, b:T3){}
    paramsFunc6(a:/* @@ label7 */T2, b:T3){}   //CTE
    paramsFunc7(a:T1, b:/* @@ label8 */T2, c:T3){}   //CTE
}


/* @@@ label1 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label2 Error Semantic error ESE0331: Type Parameter 'T2' is declared as 'out' but occurs in 'invariant' position. */
/* @@@ label3 Error Semantic error ESE0332: Type Parameter 'T1' is declared as 'in' but occurs in 'out' position. */
/* @@@ label4 Error Semantic error ESE0332: Type Parameter 'T1' is declared as 'in' but occurs in 'out' position. */
/* @@@ label5 Error Semantic error ESE0333: Type Parameter 'T2' is declared as 'out' but occurs in 'in' position. */
/* @@@ label6 Error Semantic error ESE0333: Type Parameter 'T2' is declared as 'out' but occurs in 'in' position. */
/* @@@ label7 Error Semantic error ESE0333: Type Parameter 'T2' is declared as 'out' but occurs in 'in' position. */
/* @@@ label8 Error Semantic error ESE0333: Type Parameter 'T2' is declared as 'out' but occurs in 'in' position. */