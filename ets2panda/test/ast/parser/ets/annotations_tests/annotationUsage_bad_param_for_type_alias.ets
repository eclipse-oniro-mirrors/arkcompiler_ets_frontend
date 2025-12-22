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

class A{}

@Retention("SOURCE")
@interface MyAnno {
    testProperty1: string
}

@MyAnno({testProperty1: /* @@ label1 */A})
type t1 = int

function main(): void {
    @MyAnno({testProperty1: A})
    /* @@ label2 */type t2 = int
}



/* @@@ label1 Error Syntax error ESY0318: Class cannot be used as object. */
/* @@@ label1 Error Semantic error ESE0318: Type 'A' cannot be assigned to type 'String' */
/* @@@ label1 Error Semantic error ESE0012: Invalid value for annotation field, expected a constant literal. */
/* @@@ label2 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
/* @@@ label2 Error Syntax error ESY0028: Annotations are not allowed on this type of declaration. */
