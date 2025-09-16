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

// Annotation declaration:
@Retention("SOURCE")
@interface Anno {
    name: string = "Jim"
    id: int = 1
}

@Retention("SOURCE")
@interface Anno2 {
    name: string
}

@Anno
type t1 = int

@Anno()
type t2 = int[]

@Anno({name : "2"})
type t3 = string

@Anno({id : 1})
type t4 = boolean

@Anno({name : "2", id : 1})
@Anno2({name : "ab"})
type t5 = double

// Annotation use:
function main(): void {
    @Anno
    type t6 = int[]

    @Anno()
    type t7 = int

    @Anno({name : "2"})
    type t8 = Array<int>

    @Anno({id : 1})
    type t9<T> = Array<T>

    @Anno({name : "2", id : 1})
    @Anno2({name : "ab"})
    type t10 = string[]
}


/* @@? 47:5 Error SyntaxError: Illegal start of Type Alias expression. */
/* @@? 47:5 Error SyntaxError: Annotations are not allowed on this type of declaration. */
/* @@? 50:5 Error SyntaxError: Illegal start of Type Alias expression. */
/* @@? 50:5 Error SyntaxError: Annotations are not allowed on this type of declaration. */
/* @@? 53:5 Error SyntaxError: Illegal start of Type Alias expression. */
/* @@? 53:5 Error SyntaxError: Annotations are not allowed on this type of declaration. */
/* @@? 56:5 Error SyntaxError: Illegal start of Type Alias expression. */
/* @@? 56:5 Error SyntaxError: Annotations are not allowed on this type of declaration. */
/* @@? 60:5 Error SyntaxError: Illegal start of Type Alias expression. */
/* @@? 60:5 Error SyntaxError: Annotations are not allowed on this type of declaration. */
