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

/* @@? 16:9 Error Semantic error ESE0190: A is not abstract and does not override abstract method f5(): Promise<void> in A */
/* @@? 17:12 Error Syntax error ESY0115: Duplicated modifier is not allowed. */
/* @@? 17:20 Error Semantic error ESE0017: Only abstract or native methods can't have body. */
/* @@? 18:12 Error Syntax error ESY0115: Duplicated modifier is not allowed. */
/* @@? 18:30 Error Semantic error ESE0083: Native, Abstract and Declare methods cannot have body. */
/* @@? 22:12 Error Syntax error ESY0120: Access modifier must precede field and method modifiers. */
/* @@? 22:36 Error Semantic error ESE0047: Invalid method modifier(s): an abstract method can't have private, override, static, final or native modifier. */
/* @@? 22:46 Error Semantic error ESE0083: Native, Abstract and Declare methods cannot have body. */
/* @@? 26:18 Error Syntax error ESY0116: Native method cannot be async. */
/* @@? 27:20 Error Syntax error ESY0118: Abstract method cannot be async. */
/* @@? 27:20 Error Semantic error ESE0019: Non abstract class has abstract method. */
/* @@? 35:5 Error Syntax error ESY0034: Only one static block is allowed in one namespace or class. */
/* @@? 38:10 Error Syntax error ESY0115: Duplicated modifier is not allowed. */
/* @@? 41:11 Error Syntax error ESY0221: The special predefined method '$_get' should have exactly one required parameter. */
/* @@? 41:11 Error Syntax error ESY0220: The special predefined method '$_get' cannot be asynchronous. */
/* @@? 41:16 Error Semantic error ESE0001: Return type of async function must be 'Promise'. */
/* @@? 45:11 Error Syntax error ESY0220: The special predefined method '$_set' cannot be asynchronous. */
/* @@? 45:11 Error Syntax error ESY0222: The special predefined method '$_set' should have exactly two required parameter. */
/* @@? 45:16 Error Semantic error ESE0017: Only abstract or native methods can't have body. */
/* @@? 47:11 Error Syntax error ESY0220: The special predefined method '$_iterator' cannot be asynchronous. */
/* @@? 47:11 Error Syntax error ESY0223: The special predefined method '$_iterator' should not have parameters. */
/* @@? 47:21 Error Semantic error ESE0096: The return type of '$_iterator' must be a type that implements Iterator interface. */
/* @@? 47:21 Error Semantic error ESE0001: Return type of async function must be 'Promise'. */
/* @@? 54:21 Error Semantic error ESE0153: Initializers are not allowed in ambient contexts: x */
/* @@? 54:23 Error Syntax error ESY0125: Initializers are not allowed in ambient contexts. */
/* @@? 58:1 Error Semantic error ESE0143: Unresolved reference declare */
/* @@? 58:9 Error Syntax error ESY0227: Unexpected token 'native'. */
/* @@? 58:16 Error Syntax error ESY0203: 'native' flags must be used for functions only at top-level. */
/* @@? 58:22 Error Semantic error ESE0018: Native and Declare methods should have explicit return type. */
/* @@? 58:24 Error Syntax error ESY0228: Unexpected token, expected '('. */
/* @@? 58:25 Error Syntax error ESY0112: Unexpected token, expected an identifier. */
/* @@? 58:25 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */
/* @@? 58:25 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 58:25 Error Syntax error ESY0227: Unexpected token '}'. */
/* @@? 61:5 Error Syntax error ESY0040: Illegal start of CLASS expression. */
/* @@? 62:9 Error Syntax error ESY0030: Local class or interface declaration members can not have access modifies. */
/* @@? 65:5 Error Syntax error ESY0040: Illegal start of INTERFACE expression. */
/* @@? 66:9 Error Syntax error ESY0030: Local class or interface declaration members can not have access modifies. */
/* @@? 66:18 Error Syntax error ESY0032: Private interface methods must have body. */
/* @@? 67:9 Error Syntax error ESY0224: Identifier expected, got 'static'. */
/* @@? 67:16 Error Syntax error ESY0228: Unexpected token, expected ','. */
/* @@? 67:16 Error Syntax error ESY0122: Identifier expected. */
/* @@? 67:16 Error Syntax error ESY0224: Identifier expected, got 'static'. */
/* @@? 72:5 Error Syntax error ESY0224: Identifier expected, got 'class'. */
/* @@? 72:13 Error Syntax error ESY0119: Interface fields must have type annotation. */
/* @@? 72:14 Error Syntax error ESY0138: Invalid Type. */
/* @@? 78:8 Error Syntax error ESY0038: Label must be followed by a loop statement. */
/* @@? 78:8 Error Semantic error ESE0144: Type name 'number' used in the wrong context */
/* @@? 79:1 Error Syntax error ESY0227: Unexpected token '}'. */
