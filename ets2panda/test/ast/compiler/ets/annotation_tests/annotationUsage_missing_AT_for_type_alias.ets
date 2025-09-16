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
@interface MyAnno {
    testProperty1: string
}

MyAnno({testProperty1: ""})
type t1 = int

function main() : void {
    MyAnno({testProperty1: ""})
    type t2 = int
}


/* @@? 19:1 Error TypeError: Annotation missing '@' symbol before annotation name.  */
/* @@? 19:1 Error TypeError: This expression is not callable.  */
/* @@? 23:5 Error TypeError: Annotation missing '@' symbol before annotation name.  */
/* @@? 23:5 Error TypeError: This expression is not callable.  */
/* @@? 24:5 Error SyntaxError: Illegal start of Type Alias expression.  */
