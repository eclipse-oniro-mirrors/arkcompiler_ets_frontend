/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

class C {
    public static foo: int = 10
    public foo: int = 20
}

class D extends C {
    public static foo: int = 30
    public foo: int = 40
}

class E {
    public static foo: int = 50
}

class F extends E {
    public foo: int = 60
}

class G {
    public foo: int = 70
}

class H extends G {
    public static foo: int = 80
}

function main(): void {
    arktest.assertEQ(C.foo, 10)
    arktest.assertEQ(new C().foo, 20)
    arktest.assertEQ(D.foo, 30)
    arktest.assertEQ(new D().foo, 40)
    arktest.assertEQ(F.foo, 50)
    arktest.assertEQ(new F().foo, 60)
    arktest.assertEQ(H.foo, 80)
    arktest.assertEQ(new H().foo, 70)
}
