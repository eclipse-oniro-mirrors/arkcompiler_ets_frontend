/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
    public static func(): number {
        return 1;
    }
    public func(): number {
        return 2;
    }
    public static withArg(value: int): int {
        return 100 + value;
    }
    public withArg(value: int): int {
        return 10 + value;
    }
}

class B {
    public func(): number {
        return 1;
    }
    public static func(): number {
        return 2;
    }
}

class C extends A {
    public callBaseFunc(): number {
        return super.func();
    }
    public callBaseWithArg(): int {
        return super.withArg(3);
    }
}

function main(){
    let a = new A();
    arktest.assertEQ(A.func(), 1);
    arktest.assertEQ(a.func(), 2);
    let b = new B();
    arktest.assertEQ(b.func(), 1);
    arktest.assertEQ(B.func(), 2);
    arktest.assertEQ(new C().callBaseFunc(), 2);
    arktest.assertEQ(new C().callBaseWithArg(), 13);
}
