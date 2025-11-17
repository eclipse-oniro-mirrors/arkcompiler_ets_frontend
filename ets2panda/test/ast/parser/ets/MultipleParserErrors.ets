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
        throw new Error();
    } catch () {
        //Do something.
    }
    try {
        throw new Error();
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

/* @@? 18:16 Error Syntax error ESY0096: Optional variable is not allowed in for of statements. */
/* @@? 28:29 Error Semantic error ESE0083: Native, Abstract and Declare methods cannot have body. */
/* @@? 34:14 Error Syntax error ESY0201: The modifier for a constructor should be limited to access modifiers (private, internal, protected, public), and 'native' modifiers. */
/* @@? 37:33 Error Syntax error ESY0227: Unexpected token 'rethrows'. */
/* @@? 37:33 Error Semantic error ESE0143: Unresolved reference rethrows */
/* @@? 39:14 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 39:14 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 39:14 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 39:24 Error Syntax error ESY0227: Unexpected token 'int'. */
/* @@? 39:27 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 39:29 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 39:31 Error Syntax error ESY0163: return keyword should be used in function body. */
/* @@? 39:38 Error Semantic error ESE0092: All return statements in the function should be empty or have a value. */
/* @@? 41:1 Error Semantic error ESE0143: Unresolved reference type */
/* @@? 41:6 Error Syntax error ESY0227: Unexpected token '123'. */
/* @@? 41:6 Error Semantic error ESE0025: Invalid left-hand side of assignment expression */
/* @@? 41:10 Error Syntax error ESY0072: Invalid left-hand side in assignment expression. */
/* @@? 41:12 Error Syntax error ESY0227: Unexpected token 'int'. */
/* @@? 43:6 Error Syntax error ESY0227: Unexpected token 'null'. */
/* @@? 43:6 Error Semantic error ESE0025: Invalid left-hand side of assignment expression */
/* @@? 43:11 Error Syntax error ESY0072: Invalid left-hand side in assignment expression. */
/* @@? 43:13 Error Syntax error ESY0227: Unexpected token 'byte'. */
/* @@? 45:6 Error Syntax error ESY0227: Unexpected token 'this'. */
/* @@? 45:6 Error Semantic error ESE0202: 'this' cannot be referenced from a static context */
/* @@? 45:6 Error Semantic error ESE0025: Invalid left-hand side of assignment expression */
/* @@? 45:11 Error Syntax error ESY0072: Invalid left-hand side in assignment expression. */
/* @@? 47:8 Error Syntax error ESY0105: Variable must be initialized or it's type must be declared. */
/* @@? 47:8 Error Semantic error ESE0363: Missing initializer in const declaration */
/* @@? 49:13 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 51:25 Error Syntax error ESY0067: Rest parameter must be the last formal parameter. */
/* @@? 51:27 Error Syntax error ESY0227: Unexpected token '...'. */
/* @@? 51:30 Error Syntax error ESY0227: Unexpected token 'p'. */
/* @@? 51:33 Error Syntax error ESY0227: Unexpected token 'int'. */
/* @@? 51:33 Error Syntax error ESY0038: Label must be followed by a loop statement. */
/* @@? 51:38 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 51:39 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 51:41 Error Syntax error ESY0227: Unexpected token 'int'. */
/* @@? 51:45 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 52:5 Error Syntax error ESY0163: return keyword should be used in function body. */
/* @@? 52:12 Error Semantic error ESE0092: All return statements in the function should be empty or have a value. */
/* @@? 52:12 Error Semantic error ESE0143: Unresolved reference q */
/* @@? 52:23 Error Semantic error ESE0143: Unresolved reference p */
/* @@? 55:14 Error Syntax error ESY0193: Rest parameter should be either array or tuple type. */
/* @@? 59:14 Error Syntax error ESY0193: Rest parameter should be either array or tuple type. */
/* @@? 63:22 Error Syntax error ESY0193: Rest parameter should be either array or tuple type. */
/* @@? 63:26 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 67:7 Error Semantic error ESE0370: Merging declarations is not supported, please keep all definitions of classes, interfaces and enums compact in the codebase! */
/* @@? 67:7 Error Semantic error ESE0351: Variable 'A' has already been declared. */
/* @@? 68:3 Error Syntax error ESY0233: Unexpected token. A constructor, method, accessor, or property was expected. */
/* @@? 73:7 Error Semantic error ESE0370: Merging declarations is not supported, please keep all definitions of classes, interfaces and enums compact in the codebase! */
/* @@? 73:7 Error Semantic error ESE0351: Variable 'A' has already been declared. */
/* @@? 74:11 Error Syntax error ESY0233: Unexpected token. A constructor, method, accessor, or property was expected. */
/* @@? 77:20 Error Semantic error ESE0179: Interface expected here. */
/* @@? 77:22 Error Semantic error ESE0070: 'I' type does not exist. */
/* @@? 78:22 Error Semantic error ESE0136: Method fee(): Int in B not overriding any method */
/* @@? 83:7 Error Semantic error ESE0370: Merging declarations is not supported, please keep all definitions of classes, interfaces and enums compact in the codebase! */
/* @@? 83:7 Error Semantic error ESE0351: Variable 'A' has already been declared. */
/* @@? 84:3 Error Syntax error ESY0233: Unexpected token. A constructor, method, accessor, or property was expected. */
/* @@? 92:7 Error Semantic error ESE0351: Variable 'A' has already been declared. */
/* @@? 92:7 Error Semantic error ESE0370: Merging declarations is not supported, please keep all definitions of classes, interfaces and enums compact in the codebase! */
/* @@? 93:3 Error Syntax error ESY0233: Unexpected token. A constructor, method, accessor, or property was expected. */
/* @@? 103:14 Error Syntax error ESY0193: Rest parameter should be either array or tuple type. */
/* @@? 104:21 Error Syntax error ESY0193: Rest parameter should be either array or tuple type. */
/* @@? 115:26 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 115:26 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 115:26 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 115:26 Error Syntax error ESY0227: Unexpected token 'case'. */
/* @@? 115:30 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 115:32 Error Syntax error ESY0227: Unexpected token 'U'. */
/* @@? 115:32 Error Semantic error ESE0143: Unresolved reference U */
/* @@? 115:33 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 115:34 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 115:36 Error Syntax error ESY0227: Unexpected token 'T'. */
/* @@? 115:36 Error Semantic error ESE0143: Unresolved reference T */
/* @@? 115:38 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 116:5 Error Syntax error ESY0163: return keyword should be used in function body. */
/* @@? 116:12 Error Semantic error ESE0092: All return statements in the function should be empty or have a value. */
/* @@? 119:1 Error Semantic error ESE0131: Function foo with this assembly signature already declared. */
/* @@? 119:26 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 119:26 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 119:26 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 119:26 Error Syntax error ESY0227: Unexpected token 'case'. */
/* @@? 119:30 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 119:32 Error Syntax error ESY0227: Unexpected token 'U'. */
/* @@? 119:33 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 119:34 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 119:36 Error Syntax error ESY0227: Unexpected token 'T'. */
/* @@? 119:38 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 120:5 Error Syntax error ESY0163: return keyword should be used in function body. */
/* @@? 120:12 Error Semantic error ESE0092: All return statements in the function should be empty or have a value. */
/* @@? 123:5 Error Syntax error ESY0224: Identifier expected, got ','. */
/* @@? 123:6 Error Syntax error ESY0105: Variable must be initialized or it's type must be declared. */
/* @@? 123:6 Error Syntax error ESY0227: Unexpected token 'abc'. */
/* @@? 123:6 Error Semantic error ESE0143: Unresolved reference abc */
/* @@? 125:1 Error Semantic error ESE0399: Method declaration `foo` must all ambient or non-ambient */
/* @@? 127:1 Error Syntax error ESY0106: The modifier async cannot be used in an ambient context. */
/* @@? 132:14 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 137:16 Error Syntax error ESY0230: Expected '{', got '='. */
/* @@? 137:16 Error Syntax error ESY0227: Unexpected token '='. */
/* @@? 137:16 Error Syntax error ESY0101: Catch clause variable cannot have an initializer. */
/* @@? 137:16 Error Syntax error ESY0230: Expected '{', got '='. */
/* @@? 137:16 Error Syntax error ESY0230: Expected ')', got '='. */
/* @@? 137:18 Error Syntax error ESY0227: Unexpected token '0'. */
/* @@? 137:19 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 137:21 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 141:5 Error Semantic error ESE0127: No matching call signature for assertTrue(..., undefined) */
/* @@? 141:24 Error Semantic error ESE0289: This expression is not callable. */
/* @@? 145:18 Error Semantic error ESE0052: A is abstract therefore cannot be instantiated. */
/* @@? 146:5 Error Semantic error ESE0127: No matching call signature for assertTrue(Boolean) */
/* @@? 146:28 Error Semantic error ESE0087: Property 'getInner' does not exist on type 'A' */
/* @@? 148:25 Error Semantic error ESE0052: A is abstract therefore cannot be instantiated. */
/* @@? 149:24 Error Semantic error ESE0052: A is abstract therefore cannot be instantiated. */
/* @@? 151:20 Error Semantic error ESE0371: Cannot find type 'D0'. */
/* @@? 151:29 Error Semantic error ESE0371: Cannot find type 'D0'. */
/* @@? 157:16 Error Semantic error ESE0052: A is abstract therefore cannot be instantiated. */
/* @@? 158:5 Error Semantic error ESE0127: No matching call signature for assertTrue(Boolean) */
/* @@? 158:26 Error Semantic error ESE0087: Property 'getInner' does not exist on type 'A' */
/* @@? 161:9 Error Syntax error ESY0316: Hard keyword 'let' cannot be used as identifier */
/* @@? 161:9 Error Syntax error ESY0224: Identifier expected, got 'let'. */
/* @@? 162:9 Error Syntax error ESY0316: Hard keyword 'const' cannot be used as identifier */
/* @@? 162:9 Error Syntax error ESY0224: Identifier expected, got 'const'. */
/* @@? 163:9 Error Syntax error ESY0316: Hard keyword 'new' cannot be used as identifier */
/* @@? 163:9 Error Syntax error ESY0224: Identifier expected, got 'new'. */
/* @@? 164:5 Error Semantic error ESE0289: This expression is not callable. */
/* @@? 165:5 Error Semantic error ESE0289: This expression is not callable. */
/* @@? 166:5 Error Semantic error ESE0127: No matching call signature for f() */
/* @@? 166:5 Error Semantic error ESE0124: Expected 1 arguments, got 0. */
/* @@? 297:1 Error Syntax error ESY0230: Expected '}', got 'end of stream'. */
