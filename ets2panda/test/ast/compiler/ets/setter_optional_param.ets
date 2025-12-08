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

type U = string | undefined
type V = string | number

class C1 {
    set foo1(x: string = "") { }
    set foo2(x?: string) { }
    set foo3(x?: string = "") { }
    set foo4(x: string | undefined = undefined) { }
    set foo5(x: string | undefined = "") { }
    set foo6(x: string | number = "") { }
    set foo7(x: U = "") { }
    set foo8(x: U = undefined) { }
    set foo9(x: V = 42) { }
}

class C2 {
    static set foo1(x: string = "") { }
    static set foo2(x?: string) { }
    static set foo3(x?: string = "") { }
    static set foo4(x: string | undefined = undefined) { }
    static set foo5(x: string | undefined = "") { }
    static set foo6(x: string | number = "") { }
    static set foo7(x: U = "") { }
    static set foo8(x: U = undefined) { }
    static set foo9(x: V = 42) { }
}

interface I1 {
    set foo1(x: string = "")
    set foo2(x?: string)
    set foo3(x?: string = "")
    set foo4(x: string | undefined = undefined)
    set foo5(x: string | undefined = "")
    set foo6(x: string | number = "")
    set foo7(x: U = "")
    set foo8(x: U = undefined)
    set foo9(x: V = 42)
}

/* @@? 20:14 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 21:14 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 22:14 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 22:27 Error SyntaxError: Not enable default value with default undefined. */
/* @@? 23:14 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 24:14 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 25:14 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 26:14 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 27:14 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 28:14 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 32:21 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 33:21 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 34:21 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 34:34 Error SyntaxError: Not enable default value with default undefined. */
/* @@? 35:21 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 36:21 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 37:21 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 38:21 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 39:21 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 40:21 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 44:14 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 45:14 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 46:14 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 46:27 Error SyntaxError: Not enable default value with default undefined. */
/* @@? 47:14 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 48:14 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 49:14 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 50:14 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 51:14 Error SyntaxError: Setter cannot have optional parameter. */
/* @@? 52:14 Error SyntaxError: Setter cannot have optional parameter. */
