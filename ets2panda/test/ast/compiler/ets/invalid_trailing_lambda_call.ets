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
 
function foo1(p:Any){}
function foo2(p:Required){}
function foo3(p:Partial){}
function foo4(p:Readonly){}
function foo5(p:FixedArray){}

foo1(){}
foo2(){}
foo3(){}
foo4(){}
foo5(){}

/* @@? 17:17 Error Syntax error ESY0242: 'Required' is reserved and cannot be used as a variable/type name */
/* @@? 18:17 Error Syntax error ESY0242: 'Partial' is reserved and cannot be used as a variable/type name */
/* @@? 19:17 Error Syntax error ESY0242: 'Readonly' is reserved and cannot be used as a variable/type name */
/* @@? 20:17 Error Semantic error ESE0277: FixedArray<T> must have only one type parameter. */
/* @@? 23:1 Error Semantic error ESE0124: Expected 1 arguments, got 0. */
/* @@? 23:1 Error Semantic error ESE0127: No matching call signature for foo2() */
/* @@? 24:1 Error Semantic error ESE0124: Expected 1 arguments, got 0. */
/* @@? 24:1 Error Semantic error ESE0127: No matching call signature for foo3() */
/* @@? 25:1 Error Semantic error ESE0124: Expected 1 arguments, got 0. */
/* @@? 25:1 Error Semantic error ESE0127: No matching call signature for foo4() */
/* @@? 26:1 Error Semantic error ESE0124: Expected 1 arguments, got 0. */
/* @@? 26:1 Error Semantic error ESE0127: No matching call signature for foo5() */
