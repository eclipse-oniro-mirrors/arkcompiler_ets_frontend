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

class A123<in T1,out T2,T3>{
    field1:/* @@ label1 */T1|string|undefined;   //CTE
    field2:/* @@ label2 */T2|string|undefined;   //CTE
    field3:T3|string|undefined;
    field4:/* @@ label3 */T1|T2|undefined;   //CTE
    field5:/* @@ label4 */T1|T3|undefined;   //CTE
    field6:/* @@ label5 */T2|T3|undefined;   //CTE
    field7:/* @@ label6 */T1|T2|T3;   //CTE

    readonly readonlyfield1:/* @@ label7 */T1|string|undefined;   //CTE
    readonly readonlyfield2:T2|string|undefined;
    readonly readonlyfield3:T3|string|undefined;
    readonly readonlyfield4:/* @@ label8 */T1|T2|undefined;   //CTE
    readonly readonlyfield5:/* @@ label9 */T1|T3|undefined;   //CTE
    readonly readonlyfield6:T2|T3|undefined;
    readonly readonlyfield7:/* @@ label10 */T1|T2|T3;   //CTE

    returnTypeFunc1():/* @@ label11 */T1|string|undefined{   //CTE
        return this.readonlyfield1;
    }

    returnTypeFunc2():T2|string|undefined{
        return this.readonlyfield2;
    }

    returnTypeFunc3():T3|string|undefined{
        return this.readonlyfield3;
    }

    returnTypeFunc4():/* @@ label12 */T1|T2|undefined{   //CTE
        return this.readonlyfield4;
    }

    returnTypeFunc5():/* @@ label13 */T1|T3|undefined{   //CTE
        return this.readonlyfield5;
    }

    returnTypeFunc6():T2|T3|undefined{
        return this.readonlyfield6;
    }

     returnTypeFunc7():/* @@ label14 */T1|T2|T3{   //CTE
        return this.readonlyfield7;
    }

    paramsFunc1(a:T1|string|undefined){}
    paramsFunc2(a:/* @@ label15 */T2|string|undefined){}   //CTE
    paramsFunc3(a:T3|string|undefined){}
    paramsFunc4(a:/* @@ label16 */T1|T2|undefined){}   //CTE
    paramsFunc5(a:T1|T3|undefined){}
    paramsFunc6(a:/* @@ label17 */T2|T3|undefined){}   //CTE
    paramsFunc7(a:/* @@ label18 */T1|T2|T3){}   //CTE
}


/* @@@ label1 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label2 Error Semantic error ESE0331: Type Parameter 'T2' is declared as 'out' but occurs in 'invariant' position. */
/* @@@ label3 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label3 Error Semantic error ESE0331: Type Parameter 'T2' is declared as 'out' but occurs in 'invariant' position. */
/* @@@ label4 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label5 Error Semantic error ESE0331: Type Parameter 'T2' is declared as 'out' but occurs in 'invariant' position. */
/* @@@ label6 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label6 Error Semantic error ESE0331: Type Parameter 'T2' is declared as 'out' but occurs in 'invariant' position. */
/* @@@ label7 Error Semantic error ESE0332: Type Parameter 'T1' is declared as 'in' but occurs in 'out' position. */
/* @@@ label8 Error Semantic error ESE0332: Type Parameter 'T1' is declared as 'in' but occurs in 'out' position. */
/* @@@ label9 Error Semantic error ESE0332: Type Parameter 'T1' is declared as 'in' but occurs in 'out' position. */
/* @@@ label10 Error Semantic error ESE0332: Type Parameter 'T1' is declared as 'in' but occurs in 'out' position. */
/* @@@ label11 Error Semantic error ESE0332: Type Parameter 'T1' is declared as 'in' but occurs in 'out' position. */
/* @@@ label12 Error Semantic error ESE0332: Type Parameter 'T1' is declared as 'in' but occurs in 'out' position. */
/* @@@ label13 Error Semantic error ESE0332: Type Parameter 'T1' is declared as 'in' but occurs in 'out' position. */
/* @@@ label14 Error Semantic error ESE0332: Type Parameter 'T1' is declared as 'in' but occurs in 'out' position. */
/* @@@ label15 Error Semantic error ESE0333: Type Parameter 'T2' is declared as 'out' but occurs in 'in' position. */
/* @@@ label16 Error Semantic error ESE0333: Type Parameter 'T2' is declared as 'out' but occurs in 'in' position. */
/* @@@ label17 Error Semantic error ESE0333: Type Parameter 'T2' is declared as 'out' but occurs in 'in' position. */
/* @@@ label18 Error Semantic error ESE0333: Type Parameter 'T2' is declared as 'out' but occurs in 'in' position. */