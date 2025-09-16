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
    public a:int;
    constructor(param: int=101) {
      this.a = param;
    }
    public SetA(param: int = 99) {
        this.a = param;
    }
}

class C extends A {
    static fs:int=102
    fc:int=10001;
    c:int;
    public setC (param:int=this.fc) : void {
        this.c = param;
    }
    public getC() : int {
        return this.c;
    }
    static foo(param: int=C.fs) : int {
      return C.fs+param;
    }
}

function main(): void {
    let b: C = new C();
    arktest.assertEQ(b.a, 101);
    b.setC();
    arktest.assertEQ(b.getC(), 10001);
    arktest.assertEQ(C.foo(), (C.fs*2));
}
