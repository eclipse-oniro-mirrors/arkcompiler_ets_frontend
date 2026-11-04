/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

abstract class A {
    abstract foo(): A;
}

interface G {
    bar(): void;
}

class D extends A implements G{
    override foo(): D {
        return this;
    }
    override bar(): void {
        return;
    }
}

class C {

}

class B extends A {
    override foo(): D {
        let classD: D = new D();
        return classD;
    }
    foo(a: int): C {
        let classC: C = new C();
        return classC;
    }
}
