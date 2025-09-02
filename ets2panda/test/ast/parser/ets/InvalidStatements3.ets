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

try {
    let x = 77
} catch(a: exception)
}

try
    let x = 55
} finally (x)
}


while {}

while x > 0 {}

let a = let x = 77.0;

function f(x: number, ... args: number[]) {
    "use strict"
}

function foo() {
    private @annotate
    let a: number = 10;
}

for (let i = 1 in [0, 1, 2]) {}

/* @@? 18:9 Error TypeError: Argument must be an instance of 'Exception' or 'Error' */
/* @@? 18:12 Error TypeError: Cannot find type 'exception'. */
/* @@? 19:1 Error SyntaxError: Expected '{', got '}'. */
/* @@? 22:5 Error SyntaxError: Expected '{', got 'let'. */
/* @@? 23:11 Error SyntaxError: Expected '{', got '('. */
/* @@? 23:11 Error TypeError: Unresolved reference x */
/* @@? 27:7 Error SyntaxError: Expected '(', got '{'. */
/* @@? 27:7 Error TypeError: need to specify target type for class composite */
/* @@? 29:1 Error SyntaxError: Expected ')', got 'while'. */
/* @@? 29:7 Error SyntaxError: Expected '(', got 'identification literal'. */
/* @@? 29:13 Error SyntaxError: Expected ')', got '{'. */
/* @@? 31:9 Error SyntaxError: Unexpected token 'let'. */
/* @@? 31:13 Error SyntaxError: Unexpected token 'x'. */
/* @@? 34:5 Error SyntaxError: Illegal 'use strict' directive in function with non-simple parameter list. */
/* @@? 38:5 Error SyntaxError: Annotation declaration can not have access modifier. */
/* @@? 38:5 Error SyntaxError: Unexpected token 'private'. */
/* @@? 38:13 Error SyntaxError: Unexpected token '@'. */
/* @@? 38:14 Error TypeError: Cannot find type 'annotate'. */
/* @@? 42:16 Error SyntaxError: for-in loop variable declaration may not have an initializer. */
/* @@? 42:16 Error SyntaxError: 'for ... in' loop is not supported, please use regular 'for' or 'for ... of ...' loop to iterate through arrays and iterable objects. */
