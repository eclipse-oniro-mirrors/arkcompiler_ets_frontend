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

class Base {
    methodOne(this.ownerType!, this.name): Base {
        return p;
    }
}

class Derived extends Base {
    override methodOne(p: Derived): Base {
        return p;
    }
}

function main(): void {
    let a = new Base();
    let b = new Derived()

    let resultA = a.methodOne(a);
    let resultB = b.methodOne(b);

    arktest.assertEQ(resultA, resultB);
}


/* @@? 17:14 Error Semantic error ESE0017: Only abstract or native methods can't have body. */
/* @@? 17:19 Error Syntax error ESY0229: Unexpected token, expected ',' or ')'. */
/* @@? 17:19 Error Syntax error ESY0227: Unexpected token '.'. */
/* @@? 17:19 Error Syntax error ESY0099: The function parameter 'this' must explicitly specify the typeAnnotation. */
/* @@? 17:30 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 17:30 Error Syntax error ESY0227: Unexpected token ','. */
/* @@? 17:32 Error Syntax error ESY0227: Unexpected token 'this'. */
/* @@? 17:36 Error Syntax error ESY0227: Unexpected token '.'. */
/* @@? 17:41 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 17:41 Error Syntax error ESY0227: Unexpected token ')'. */
/* @@? 17:42 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 17:48 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 17:49 Error Syntax error ESY0227: Unexpected token '{'. */
/* @@? 18:9 Error Syntax error ESY0227: Unexpected token 'return'. */
/* @@? 18:17 Error Syntax error ESY0121: Field type annotation expected. */
/* @@? 20:1 Error Syntax error ESY0227: Unexpected token '}'. */
/* @@? 23:23 Error Semantic error ESE0136: Method methodOne(p: Derived): Base in Derived not overriding any method */
/* @@? 32:21 Error Semantic error ESE0087: Property 'methodOne' does not exist on type 'Base' */