/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http: *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

declare type type_tuple_union = [string, boolean, number] | (this.mnum === other.mnum && this.mden === other.mden
)[]

/* @@? 15:62 Error SyntaxError: A 'this' type is available only as return type in a non-static method of a class or struct and extension functions. */
/* @@? 15:66 Error SyntaxError: Unexpected token, expected ')'. */
/* @@? 15:67 Error TypeError: Unresolved reference mnum */
/* @@? 15:76 Error TypeError: Unresolved reference other */
/* @@? 15:90 Error TypeError: 'this' cannot be referenced from a static context */
/* @@? 15:95 Error TypeError: Property 'mden' does not exist on type 'ETSGLOBAL' */
/* @@? 16:1 Error SyntaxError: Unexpected token ')'. */
/* @@? 16:1 Error TypeError: Indexed access is not supported for such expression type. */
/* @@? 16:3 Error SyntaxError: Unexpected token ']'. */
/* @@? 28:1 Error SyntaxError: Unexpected token, expected ']'. */
