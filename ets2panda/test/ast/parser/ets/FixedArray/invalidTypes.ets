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

class Class1 {
  public x: int;
  public constructor(): int {
    this.x = 15;
    return;
  }
}

let var1: [FixedArray<...number>, FixedArray<...string>];

let var2: [FixedArray<...number>, string];

let var3: [number?];

let var4: [number: ...int];

let var5: [number number];

class Class2 {
  static method1(): this {}
}

let var6: [a0: , a1: ];

/* @@? 18:23 Error SyntaxError: Type annotation isn't allowed for constructor. */
/* @@? 24:12 Error TypeError: FixedArray<T> must have only one type parameter. */
/* @@? 24:23 Error SyntaxError: Invalid Type. */
/* @@? 24:23 Error SyntaxError: Unexpected token, expected '>'. */
/* @@? 24:23 Error SyntaxError: Unexpected token, expected ',' or ']'. */
/* @@? 24:23 Error SyntaxError: Unexpected token '>'. */
/* @@? 24:26 Error SyntaxError: Unexpected token 'number'. */
/* @@? 24:26 Error TypeError: Type name 'number' used in the wrong context */
/* @@? 24:33 Error SyntaxError: Unexpected token ','. */
/* @@? 24:35 Error SyntaxError: Unexpected token 'FixedArray'. */
/* @@? 24:35 Error TypeError: Unresolved reference FixedArray */
/* @@? 24:46 Error SyntaxError: Unexpected token '...'. */
/* @@? 24:49 Error SyntaxError: Unexpected token 'string'. */
/* @@? 24:49 Error TypeError: Type name 'string' used in the wrong context */
/* @@? 24:56 Error SyntaxError: Unexpected token ']'. */
/* @@? 26:12 Error TypeError: FixedArray<T> must have only one type parameter. */
/* @@? 26:23 Error SyntaxError: Invalid Type. */
/* @@? 26:23 Error SyntaxError: Unexpected token, expected '>'. */
/* @@? 26:23 Error SyntaxError: Unexpected token, expected ',' or ']'. */
/* @@? 26:23 Error SyntaxError: Unexpected token '>'. */
/* @@? 26:26 Error SyntaxError: Unexpected token 'number'. */
/* @@? 26:26 Error TypeError: Type name 'number' used in the wrong context */
/* @@? 26:33 Error SyntaxError: Unexpected token ','. */
/* @@? 26:35 Error SyntaxError: Unexpected token 'string'. */
/* @@? 26:35 Error TypeError: Type name 'string' used in the wrong context */
/* @@? 26:41 Error SyntaxError: Unexpected token ']'. */
/* @@? 28:18 Error SyntaxError: Unexpected token, expected ',' or ']'. */
/* @@? 28:18 Error SyntaxError: Unexpected token '?'. */
/* @@? 28:19 Error SyntaxError: Unexpected token ']'. */
/* @@? 30:18 Error SyntaxError: Unexpected token, expected ',' or ']'. */
/* @@? 30:18 Error SyntaxError: Unexpected token ':'. */
/* @@? 30:20 Error SyntaxError: Unexpected token '...'. */
/* @@? 30:23 Error SyntaxError: Unexpected token 'int'. */
/* @@? 30:26 Error SyntaxError: Unexpected token ']'. */
/* @@? 32:19 Error SyntaxError: Unexpected token 'number'. */
/* @@? 32:19 Error SyntaxError: Unexpected token, expected ',' or ']'. */
/* @@? 32:19 Error TypeError: Type name 'number' used in the wrong context */
/* @@? 32:25 Error SyntaxError: Unexpected token ']'. */
/* @@? 35:21 Error SyntaxError: A 'this' type is available only as return type in a non-static method of a class or struct and extension functions. */
/* @@? 38:12 Error TypeError: Cannot find type 'a0'. */
/* @@? 38:14 Error SyntaxError: Unexpected token ':'. */
/* @@? 38:14 Error SyntaxError: Unexpected token, expected ',' or ']'. */
/* @@? 38:16 Error SyntaxError: Unexpected token ','. */
/* @@? 38:18 Error SyntaxError: Unexpected token 'a1'. */
/* @@? 38:22 Error SyntaxError: Label must be followed by a loop statement. */
/* @@? 38:22 Error SyntaxError: Unexpected token ']'. */
