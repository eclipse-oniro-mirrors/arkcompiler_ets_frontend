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

class A{}
class B{}
class C{}
class TempAny{}

class Test1 {
    async foo111(a: A): Promise<A> {
        return a;
    }
    async foo112(a: B): Promise<B> {
        return a;
    }

    async foo113(a: C): Promise<C> {
        console.log(a)
        return a;
    }
    async overload foo{foo111,foo112,foo113}
}

class Test2 {
    static foo111(a: A): A {
        return a;
    }
    static foo112(a: B): B {
        return a;
    }

    static foo113(a: C): C {
        console.log(a)
        return a;
    }
    static overload foo{foo111,foo112,foo113}
}

class Test3 {
    private foo111(a: A): A {
        return a;
    }
    private foo112(a: B): B {
        return a;
    }

    private foo113(a: C): C {
        console.log(a)
        return a;
    }
    private overload foo{foo111,foo112,foo113}
}
