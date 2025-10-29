/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

@Retention("SOURCE")
@interface Anno{
}

// annotations for type parameter
function foo<@Anno @Anno T> (a: @Anno @Anno() T):@Anno @Anno Int{return 1}
class A<@Anno @Anno() T>{
   a: @Anno @Anno T
   bar<@Anno @Anno() T>(){}
}
let array1: @Anno @Anno() Array<Int> = new Array<Int>()


/* @@? 21:21 Error Semantic error ESE0041: Duplicate annotations are not allowed. The annotation 'Anno' has already been applied to this element. */
/* @@? 21:40 Error Semantic error ESE0041: Duplicate annotations are not allowed. The annotation 'Anno' has already been applied to this element. */
/* @@? 21:57 Error Semantic error ESE0041: Duplicate annotations are not allowed. The annotation 'Anno' has already been applied to this element. */
/* @@? 22:16 Error Semantic error ESE0041: Duplicate annotations are not allowed. The annotation 'Anno' has already been applied to this element. */
/* @@? 23:14 Error Semantic error ESE0041: Duplicate annotations are not allowed. The annotation 'Anno' has already been applied to this element. */
/* @@? 24:15 Error Semantic error ESE0041: Duplicate annotations are not allowed. The annotation 'Anno' has already been applied to this element. */
/* @@? 26:20 Error Semantic error ESE0041: Duplicate annotations are not allowed. The annotation 'Anno' has already been applied to this element. */