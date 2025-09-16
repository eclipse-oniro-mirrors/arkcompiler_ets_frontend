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

class A { }
class B { }
class C { }

class Test1 {
    foo111(a: A): A {
        return a;
    }
    foo112(a: B): B {
        return a;
    }

    foo113(a: C): C {
        return a;
    }
    overload foo{ foo111, foo112, foo113,/* @@ label1 */foo111 }
}

/* @@@ label1 Error TypeError: Duplicate overloaded method. */