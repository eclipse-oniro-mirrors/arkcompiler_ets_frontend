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

class InB<in T> {}
class OutB<out T> {}
class B<T> {}

class A123<in T1,out T2,T3>{
    readonly readonlyfield1:InB<T1>;
    readonly readonlyfield2:/* @@ label1 */InB<T2>;             //CTE
    readonly readonlyfield3:InB<T3>;
    readonly readonlyfield4:InB<T1|T3>;
    readonly readonlyfield5:/* @@ label2 */InB<[T1,T3,number]>; //CTE
    readonly readonlyfield7:/* @@ label3 */OutB<T1>;            //CTE
    readonly readonlyfield8:OutB<T2>;
    readonly readonlyfield9:OutB<T3>;
    readonly readonlyfield10:/* @@ label4 */B<T1>;              //CTE
    readonly readonlyfield11:/* @@ label5 */B<T2>;              //CTE
    readonly readonlyfield12:B<T3>;
}


/* @@@ label1 Error Semantic error ESE0333: Type Parameter 'T2' is declared as 'out' but occurs in 'in' position. */
/* @@@ label2 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label3 Error Semantic error ESE0332: Type Parameter 'T1' is declared as 'in' but occurs in 'out' position. */
/* @@@ label4 Error Semantic error ESE0331: Type Parameter 'T1' is declared as 'in' but occurs in 'invariant' position. */
/* @@@ label5 Error Semantic error ESE0331: Type Parameter 'T2' is declared as 'out' but occurs in 'invariant' position. */