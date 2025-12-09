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
    private fooPrivate(x: string) {}
    protected fooProtected(x: number) {}
    public fooPublic(x: boolean) {}

    overload foo{ fooPrivate, fooProtected, fooPublic }

    bar(){
        this.foo("abc");
        this.foo(1);
        this.foo(true);
    }
}

class Sub extends Base {
    bar() {
        /* @@ label */this.foo(/* @@ label1 */"abc");
        this.foo(1);
        this.foo(true);
    }
}

function main() {
    let a = new Base();
    /* @@ label2 */a.foo("abc");
    /* @@ label3 */a.foo(1);
    a.foo(true);
}


/* @@@ label Error Semantic error ESE0139: Signature fooPrivate(x: String): void is not visible here. */
/* @@@ label Error Semantic error ESE0127: No matching call signature for foo("abc") */
/* @@@ label2 Error Semantic error ESE0139: Signature fooPrivate(x: String): void is not visible here. */
/* @@@ label2 Error Semantic error ESE0127: No matching call signature for foo("abc") */
/* @@@ label3 Error Semantic error ESE0139: Signature fooProtected(x: Double): void is not visible here. */
/* @@@ label3 Error Semantic error ESE0127: No matching call signature for foo(Int) */
