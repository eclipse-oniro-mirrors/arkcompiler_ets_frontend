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

class A {
    static static f(): void;
    native native f2(): void {
        console.log(f);
    }

    static private abstract static f3(): int {
        return 7
    }

    native async f4();
    async abstract f5();

    static {
        let x: number = 99
    }

    static {
        let y: number = 75
    }
}

abstract abstract class B {
    x: number = 77;

    async $_get(): int {
        return 5;
    }

    async $_set(): void;

    async $_iterator(a: int): int {
        return a + 1;
    }
}

declare namespace abc {
    class C {
        x: number = 53;
    }
}

declare native class Z {}

function s(): void {
    class C {
        private x: number;
    }

    interface I {
        private f(): void;
        static static g(): void;
    }
}

interface I1 {
    class A {}

    get color(): string {
        return "red";
    }

    x: number =
}

/* @@? 16:9 Error TypeError: A is not abstract and does not override abstract method f5(): Promise<void> in A */
/* @@? 17:12 Error SyntaxError: Duplicated modifier is not allowed. */
/* @@? 17:20 Error TypeError: Only abstract or native methods can't have body. */
/* @@? 18:12 Error SyntaxError: Duplicated modifier is not allowed. */
/* @@? 18:30 Error TypeError: Native, Abstract and Declare methods cannot have body. */
/* @@? 19:21 Error TypeError: Static property 'f' must be accessed through it's class 'A' */
/* @@? 22:12 Error SyntaxError: Access modifier must precede field and method modifiers. */
/* @@? 22:36 Error TypeError: Invalid method modifier(s): an abstract method can't have private, override, static, final or native modifier. */
/* @@? 22:46 Error TypeError: Native, Abstract and Declare methods cannot have body. */
/* @@? 26:18 Error SyntaxError: Native method cannot be async. */
/* @@? 27:20 Error SyntaxError: Abstract method cannot be async. */
/* @@? 27:20 Error TypeError: Non abstract class has abstract method. */
/* @@? 35:5 Error SyntaxError: Only one static block is allowed in one namespace or class. */
/* @@? 38:10 Error SyntaxError: Duplicated modifier is not allowed. */
/* @@? 41:11 Error SyntaxError: The special predefined method '$_get' should have exactly one required parameter. */
/* @@? 41:11 Error SyntaxError: The special predefined method '$_get' cannot be asynchronous. */
/* @@? 41:16 Error TypeError: Return type of async function must be 'Promise'. */
/* @@? 45:11 Error SyntaxError: The special predefined method '$_set' should have exactly two required parameter. */
/* @@? 45:11 Error SyntaxError: The special predefined method '$_set' cannot be asynchronous. */
/* @@? 45:16 Error TypeError: Only abstract or native methods can't have body. */
/* @@? 47:11 Error SyntaxError: The special predefined method '$_iterator' cannot be asynchronous. */
/* @@? 47:11 Error SyntaxError: The special predefined method '$_iterator' should not have parameters. */
/* @@? 47:21 Error TypeError: The return type of '$_iterator' must be a type that implements Iterator interface. */
/* @@? 47:21 Error TypeError: Return type of async function must be 'Promise'. */
/* @@? 54:21 Error TypeError: Initializers are not allowed in ambient contexts: x */
/* @@? 54:23 Error SyntaxError: Initializers are not allowed in ambient contexts. */
/* @@? 58:1 Error TypeError: Unresolved reference declare */
/* @@? 58:9 Error SyntaxError: Unexpected token 'native'. */
/* @@? 58:16 Error SyntaxError: 'native' flags must be used for functions only at top-level. */
/* @@? 58:22 Error TypeError: Native and Declare methods should have explicit return type. */
/* @@? 58:24 Error SyntaxError: Unexpected token, expected '('. */
/* @@? 58:25 Error SyntaxError: Unexpected token, expected an identifier. */
/* @@? 58:25 Error SyntaxError: Parameter declaration should have an explicit type annotation. */
/* @@? 58:25 Error SyntaxError: Unexpected token, expected ',' or ')'. */
/* @@? 58:25 Error SyntaxError: Unexpected token '}'. */
/* @@? 61:5 Error SyntaxError: Illegal start of CLASS expression. */
/* @@? 62:9 Error SyntaxError: Local class or interface declaration members can not have access modifies. */
/* @@? 65:5 Error SyntaxError: Illegal start of INTERFACE expression. */
/* @@? 66:9 Error SyntaxError: Local class or interface declaration members can not have access modifies. */
/* @@? 66:18 Error SyntaxError: Private interface methods must have body. */
/* @@? 67:9 Error SyntaxError: Identifier expected, got 'static'. */
/* @@? 67:16 Error SyntaxError: Unexpected token, expected ','. */
/* @@? 67:16 Error SyntaxError: Identifier expected, got 'static'. */
/* @@? 67:16 Error SyntaxError: Identifier expected. */
/* @@? 72:5 Error SyntaxError: Identifier expected, got 'class'. */
/* @@? 72:13 Error SyntaxError: Interface fields must have type annotation. */
/* @@? 72:14 Error SyntaxError: Invalid Type. */
/* @@? 74:5 Error SyntaxError: Extension Getter can only have 1 parameter. */
/* @@? 74:25 Error SyntaxError: Extension Accessor must have a receiver. */
/* @@? 78:8 Error SyntaxError: Label must be followed by a loop statement. */
/* @@? 78:8 Error TypeError: Type name 'number' used in the wrong context */
/* @@? 79:1 Error SyntaxError: Unexpected token '}'. */
