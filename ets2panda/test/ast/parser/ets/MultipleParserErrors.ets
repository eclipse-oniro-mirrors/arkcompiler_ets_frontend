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

function foo(param? : Object)
{
   for (let i? of "blablabla") { }
}

function foo(b : int, a : int = 10, c : int = 15) : int
{
    return a + b;
}

class Door {
    private _width: int;
    native get width(): int {
        return this._width;
    }
}

abstract class A{
    abstract constructor(val: int);
};

let a: (c: int, b: int) => char rethrows;

function foo(undefined int) { return 1}

type 123 = int;

type null = byte;

type this = Object;

const x;

function f(a): void {}

function foo(...q: int[], ...p: int[]): int {
    return q.length + p.length;
}

function foo(...p: Object): Object {
    return new Object();
}

function foo(...p: int): int {
    return 1;
}

function foo(a: int, ...p): int {
    return a;
}

class A {
  class B {
    public x: int;
  }
}

class A {
  private interface I { fee(): int }
}

class B implements A.I {
  public override fee(): int {
    return 1;
  }
}

class A {
  class B {
    i: int = 1;
  }
  getInner(): B {
    return new B();
  }
}

class A {
  class B {
    foo(): int {
      return 1;
    }
  }
  getInner(): B {
    return new B();
  }
}

function foo(... a: byte): void {}
native function foo(... b: byte): long;

function min(v: double, u: double): double {
    arktest.assertTrue(false)
    return v < u ? v : u;
}

class D1 {
  public getValue(): int { return 1 }
}

function foo<T, U>(a: T, case: U): T {
    return a;
}

function foo<T, U>(a: T, case: U): T {
    return a;
}

let ,abc = 0;

declare async function foo(): Promise<void>

declare function addResult(): number;

function main(): void {
    try {
        throw new Exception();
    } catch () {
        //Do something.
    }
    try {
        throw new Exception();
    } catch (e = 0) {
        //Do something.
    }

    arktest.assertTrue(foo(), undefined);

    f(0);

    let a10: A = new A();
    arktest.assertTrue(a10.getInner().i ==  2); // #22840 arktest.assertEQ incorrect check types of operands

    let instance_A: A = new A(2, 3);
    let instanc_A: A = new A(2);

    let instance0: D0 = new D0();
    instance0.x;

    let instance1: D1 = new D1();
    instance1.getValue();

    let b: A = new A();
    arktest.assertTrue(b.getInner().foo() == 1); // #22840 arktest.assertEQ incorrect check types of operands

    min(1.0, 1.0)
    let let = 0;
    let const = 0;
    let new = 0;
    foo<Long, Boolean>(new Long(), new Boolean(true))
    foo<Long, Boolean>(new Long(), new Boolean(true))
    f();
}

/* @@? 18:14 Error SyntaxError: Optional variable is not allowed in for of statements. */
/* @@? 28:29 Error TypeError: Native, Abstract and Declare methods cannot have body. */
/* @@? 34:14 Error SyntaxError: The modifier for a constructor should be limited to access modifiers (private, internal, protected, public), and 'native' modifiers. */
/* @@? 37:33 Error SyntaxError: Unexpected token 'rethrows'. */
/* @@? 37:33 Error TypeError: Unresolved reference rethrows */
/* @@? 39:14 Error SyntaxError: Parameter declaration should have an explicit type annotation. */
/* @@? 39:14 Error SyntaxError: Unexpected token, expected an identifier. */
/* @@? 39:14 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 39:24 Error SyntaxError: Unexpected token 'int'. */
/* @@? 39:27 Error SyntaxError: Unexpected token ')'. */
/* @@? 39:29 Error SyntaxError: Unexpected token '{'. */
/* @@? 39:31 Error SyntaxError: return keyword should be used in function body. */
/* @@? 39:38 Error TypeError: All return statements in the function should be empty or have a value. */
/* @@? 41:1 Error TypeError: Unresolved reference type */
/* @@? 41:6 Error SyntaxError: Unexpected token '123'. */
/* @@? 41:6 Error TypeError: Invalid left-hand side of assignment expression */
/* @@? 41:10 Error SyntaxError: Invalid left-hand side in assignment expression. */
/* @@? 41:12 Error SyntaxError: Unexpected token 'int'. */
/* @@? 43:6 Error SyntaxError: Unexpected token 'null'. */
/* @@? 43:6 Error TypeError: Invalid left-hand side of assignment expression */
/* @@? 43:11 Error SyntaxError: Invalid left-hand side in assignment expression. */
/* @@? 43:13 Error SyntaxError: Unexpected token 'byte'. */
/* @@? 45:6 Error SyntaxError: Unexpected token 'this'. */
/* @@? 45:6 Error TypeError: 'this' cannot be referenced from a static context */
/* @@? 45:6 Error TypeError: Invalid left-hand side of assignment expression */
/* @@? 45:11 Error SyntaxError: Invalid left-hand side in assignment expression. */
/* @@? 47:8 Error SyntaxError: Variable must be initialized or it's type must be declared. */
/* @@? 47:8 Error TypeError: Missing initializer in const declaration */
/* @@? 49:13 Error SyntaxError: Parameter declaration should have an explicit type annotation. */
/* @@? 51:25 Error SyntaxError: Rest parameter must be the last formal parameter. */
/* @@? 51:27 Error SyntaxError: Unexpected token '...'. */
/* @@? 51:30 Error SyntaxError: Unexpected token 'p'. */
/* @@? 51:33 Error SyntaxError: Label must be followed by a loop statement. */
/* @@? 51:38 Error SyntaxError: Unexpected token ')'. */
/* @@? 51:39 Error SyntaxError: Unexpected token ':'. */
/* @@? 51:41 Error SyntaxError: Unexpected token 'int'. */
/* @@? 51:45 Error SyntaxError: Unexpected token '{'. */
/* @@? 52:5 Error SyntaxError: return keyword should be used in function body. */
/* @@? 52:12 Error TypeError: All return statements in the function should be empty or have a value. */
/* @@? 52:12 Error TypeError: Unresolved reference q */
/* @@? 52:23 Error TypeError: Unresolved reference p */
/* @@? 55:14 Error SyntaxError: Rest parameter should be either array or tuple type. */
/* @@? 63:26 Error SyntaxError: Parameter declaration should have an explicit type annotation. */
/* @@? 67:7 Error TypeError: Merging declarations is not supported, please keep all definitions of classes, interfaces and enums compact in the codebase! */
/* @@? 67:7 Error TypeError: Variable 'A' has already been declared. */
/* @@? 68:3 Error SyntaxError: Unexpected token. A constructor, method, accessor, or property was expected. */
/* @@? 73:7 Error TypeError: Merging declarations is not supported, please keep all definitions of classes, interfaces and enums compact in the codebase! */
/* @@? 73:7 Error TypeError: Variable 'A' has already been declared. */
/* @@? 74:11 Error SyntaxError: Unexpected token. A constructor, method, accessor, or property was expected. */
/* @@? 77:20 Error TypeError: Interface expected here. */
/* @@? 77:22 Error TypeError: 'I' type does not exist. */
/* @@? 78:22 Error TypeError: Method fee(): Int in B not overriding any method */
/* @@? 83:7 Error TypeError: Merging declarations is not supported, please keep all definitions of classes, interfaces and enums compact in the codebase! */
/* @@? 83:7 Error TypeError: Variable 'A' has already been declared. */
/* @@? 84:3 Error SyntaxError: Unexpected token. A constructor, method, accessor, or property was expected. */
/* @@? 92:7 Error TypeError: Variable 'A' has already been declared. */
/* @@? 92:7 Error TypeError: Merging declarations is not supported, please keep all definitions of classes, interfaces and enums compact in the codebase! */
/* @@? 93:3 Error SyntaxError: Unexpected token. A constructor, method, accessor, or property was expected. */
/* @@? 115:26 Error SyntaxError: Unexpected token 'case'. */
/* @@? 115:26 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 115:26 Error SyntaxError: Parameter declaration should have an explicit type annotation. */
/* @@? 115:26 Error SyntaxError: Unexpected token, expected an identifier. */
/* @@? 115:30 Error SyntaxError: Unexpected token ':'. */
/* @@? 115:32 Error SyntaxError: Unexpected token 'U'. */
/* @@? 115:32 Error TypeError: Unresolved reference U */
/* @@? 115:33 Error SyntaxError: Unexpected token ')'. */
/* @@? 115:34 Error SyntaxError: Unexpected token ':'. */
/* @@? 115:36 Error SyntaxError: Unexpected token 'T'. */
/* @@? 115:36 Error TypeError: Unresolved reference T */
/* @@? 115:38 Error SyntaxError: Unexpected token '{'. */
/* @@? 116:5 Error SyntaxError: return keyword should be used in function body. */
/* @@? 116:12 Error TypeError: All return statements in the function should be empty or have a value. */
/* @@? 119:26 Error SyntaxError: Unexpected token 'case'. */
/* @@? 119:26 Error SyntaxError: Parameter declaration should have an explicit type annotation. */
/* @@? 119:26 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 119:26 Error SyntaxError: Unexpected token, expected an identifier. */
/* @@? 119:30 Error SyntaxError: Unexpected token ':'. */
/* @@? 119:32 Error SyntaxError: Unexpected token 'U'. */
/* @@? 119:33 Error SyntaxError: Unexpected token ')'. */
/* @@? 119:34 Error SyntaxError: Unexpected token ':'. */
/* @@? 119:36 Error SyntaxError: Unexpected token 'T'. */
/* @@? 119:38 Error SyntaxError: Unexpected token '{'. */
/* @@? 120:5 Error SyntaxError: return keyword should be used in function body. */
/* @@? 120:12 Error TypeError: All return statements in the function should be empty or have a value. */
/* @@? 123:5 Error SyntaxError: Identifier expected, got ','. */
/* @@? 123:6 Error SyntaxError: Variable must be initialized or it's type must be declared. */
/* @@? 123:6 Error SyntaxError: Unexpected token 'abc'. */
/* @@? 123:6 Error TypeError: Unresolved reference abc */
/* @@? 127:1 Error SyntaxError: The modifier async cannot be used in an ambient context. */
/* @@? 132:14 Error SyntaxError: Unexpected token, expected an identifier. */
/* @@? 137:16 Error SyntaxError: Catch clause variable cannot have an initializer. */
/* @@? 137:16 Error SyntaxError: Expected ')', got '='. */
/* @@? 137:16 Error SyntaxError: Expected '{', got '='. */
/* @@? 137:16 Error SyntaxError: Unexpected token '='. */
/* @@? 137:18 Error SyntaxError: Unexpected token '0'. */
/* @@? 137:19 Error SyntaxError: Unexpected token ')'. */
/* @@? 137:21 Error SyntaxError: Unexpected token '{'. */
/* @@? 141:24 Error TypeError: This expression is not callable. */
/* @@? 145:18 Error TypeError: A is abstract therefore cannot be instantiated. */
/* @@? 146:28 Error TypeError: Property 'getInner' does not exist on type 'A' */
/* @@? 148:25 Error TypeError: A is abstract therefore cannot be instantiated. */
/* @@? 149:24 Error TypeError: A is abstract therefore cannot be instantiated. */
/* @@? 151:20 Error TypeError: Cannot find type 'D0'. */
/* @@? 151:29 Error TypeError: Cannot find type 'D0'. */
/* @@? 157:16 Error TypeError: A is abstract therefore cannot be instantiated. */
/* @@? 158:26 Error TypeError: Property 'getInner' does not exist on type 'A' */
/* @@? 161:9 Error SyntaxError: Hard keyword 'let' cannot be used as identifier */
/* @@? 161:9 Error SyntaxError: Identifier expected, got 'let'. */
/* @@? 162:9 Error SyntaxError: Identifier expected, got 'const'. */
/* @@? 162:9 Error SyntaxError: Hard keyword 'const' cannot be used as identifier */
/* @@? 163:9 Error SyntaxError: Identifier expected, got 'new'. */
/* @@? 163:9 Error SyntaxError: Hard keyword 'new' cannot be used as identifier */
/* @@? 164:5 Error TypeError: This expression is not callable. */
/* @@? 165:5 Error TypeError: This expression is not callable. */
/* @@? 166:5 Error TypeError: No matching call signature */
/* @@? 166:5 Error TypeError: Expected 1 arguments, got 0. */
/* @@? 286:1 Error SyntaxError: Expected '}', got 'end of stream'. */
