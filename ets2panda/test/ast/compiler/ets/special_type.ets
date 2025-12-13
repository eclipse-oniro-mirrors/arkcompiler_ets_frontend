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
function foo1(a:Any){}
function foo2(a:Required){}
function foo3(a:Readonly){}
function foo4(a:Partial){}
function foo5(a:FixedArray){}

foo1(()=>{})
foo2(()=>{})
foo3(()=>{})
foo4(()=>{})
foo5(()=>{})

/* @@? 16:17 Error Syntax error ESY0242: 'Required' is reserved and cannot be used as a variable/type name */
/* @@? 17:17 Error Syntax error ESY0242: 'Readonly' is reserved and cannot be used as a variable/type name */
/* @@? 18:17 Error Syntax error ESY0242: 'Partial' is reserved and cannot be used as a variable/type name */
/* @@? 19:17 Error Semantic error ESE0277: FixedArray<T> must have only one type parameter. */
/* @@? 22:1 Error Semantic error ESE0127: No matching call signature for foo2((() => void)) */
/* @@? 23:1 Error Semantic error ESE0127: No matching call signature for foo3((() => void)) */
/* @@? 24:1 Error Semantic error ESE0127: No matching call signature for foo4((() => void)) */
/* @@? 25:1 Error Semantic error ESE0127: No matching call signature for foo5((() => void)) */

