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

declare class Environment {
    static foo1(prps: {key:string}[]):void;
    static foo2(props1: {key:string}[], props2: int):void;
    static foo3(props1: , props2: int):void;
    static foo4(props:int[]):void;
    static foo5(x: <<k*, y::::, z:()()---,):void;
    static foo6(x: <<k*,):void;
    static foo7(props1: {key:string, value:int}[], props2: int[]): void;
}

/* @@? 17:23 Error SyntaxError: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@? 18:25 Error SyntaxError: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@? 19:25 Error SyntaxError: Invalid Type. */
/* @@? 21:20 Error SyntaxError: Invalid Type. */
/* @@? 21:28 Error SyntaxError: Invalid Type. */
/* @@? 21:37 Error SyntaxError: Unexpected token, expected '=>'. */
/* @@? 21:37 Error SyntaxError: Unexpected token, expected '('. */
/* @@? 21:37 Error SyntaxError: Invalid Type. */
/* @@? 22:20 Error SyntaxError: Invalid Type. */
/* @@? 23:25 Error SyntaxError: Using object literals to declare types in place is not supported. Please declare types and interfaces explicitly! */
/* @@? 23:36 Error SyntaxError: Unexpected token ','. */
