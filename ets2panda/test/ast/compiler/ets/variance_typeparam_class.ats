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

class B<in U1,out U2,U3>{}

class A<in T1,out T2,T3>{
    field1:/* @@ label1 */B<T1,T2,T3>;   //CTE 
    field2:/* @@ label2 */B<T2,T1,T3>;   //CTE
    field3:B<T3,T3,T3>;

    readonly readonlyfield1:B<T1,T2,T3>;   //Pass 
                                           //1.readonlyfield1 is read-only ,need covariant/invariance
                                           //2.B<in U1,out U2,U3> U1 is contravariant, U2 is covariant, U3 is invariance
                                           //3.B<T1,T2,T3>        T1 is contravariant, T2 is covariant, T3 is invariance
                                           //B<T1,T2,T3> is covariant
    readonly readonlyfield2:/* @@ label3 */B<T2,T1,T3>;   //CTE 
                                           //1.readonlyfield2 is read-only ,need covariant/invariance
                                           //2.B<in U1,out U2,U3> U1 is contravariant, U2 is covariant, U3 is invariance
                                           //3.B<T2,T1,T3>        T2 is covariant, T1 is contravariant, T3 is invariance
                                           //B<T1,T2,T3> is contravariant
    readonly readonlyfield3:B<T3,T3,T3>;

    returnTypeFunc1():B<T1,T2,T3>{
        return this.readonlyfield1;
    }

    returnTypeFunc2():/* @@ label4 */B<T2,T1,T3>{   //CTE
        return this.readonlyfield2;
    }

    returnTypeFunc3():B<T3,T3,T3>{
        return this.readonlyfield3;
    }

    paramsFunc1(a:/* @@ label5 */B<T1,T2,T3>){}   //CTE
    paramsFunc2(a:B<T2,T1,T3>){}
    paramsFunc3(a:B<T3,T3,T3>){}
}


/* @@@ label1 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label1 Error Semantic error ESE0331: Type Parameter 'T2' is declared as 'out' but occurs in 'invariant' position. */
/* @@@ label2 Error Semantic error ESE0331: Type Parameter 'T2' is declared as 'out' but occurs in 'invariant' position. */
/* @@@ label2 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label3 Error Semantic error ESE0333: Type Parameter 'T2' is declared as 'out' but occurs in 'in' position. */
/* @@@ label3 Error Semantic error ESE0332: Type Parameter 'T1' is declared as 'in' but occurs in 'out' position. */
/* @@@ label4 Error Semantic error ESE0333: Type Parameter 'T2' is declared as 'out' but occurs in 'in' position. */
/* @@@ label4 Error Semantic error ESE0332: Type Parameter 'T1' is declared as 'in' but occurs in 'out' position. */
/* @@@ label5 Error Semantic error ESE0332: Type Parameter 'T1' is declared as 'in' but occurs in 'out' position. */
/* @@@ label5 Error Semantic error ESE0333: Type Parameter 'T2' is declared as 'out' but occurs in 'in' position. */