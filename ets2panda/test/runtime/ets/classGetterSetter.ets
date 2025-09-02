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

interface inter {
    get my_field (): int;
    set my_field(param: int);

    get_field(): int;
}

class A implements inter {
    my_field: int = 3;

    get_field(): int {
      arktest.assertTrue(false)
      return 1;
    }
}

class B extends A{
    laca: int = 1;
    get my_field (): int{return this.laca}
    set my_field(param: int) {
        this.laca = param;
    }
    
    get_field(): int {
      return this.laca;
    }
}

function main() {
    let a0: A = new A();
    arktest.assertEQ(a0.my_field, 3)

    let a1: inter = new A();
    arktest.assertEQ(a1.my_field, 3)
    a1.my_field = 5;
    arktest.assertEQ(a1.my_field, 5)

    let a2: inter = new B();
    arktest.assertEQ(a2.my_field, a2.get_field())
    a2.my_field = 2;
    arktest.assertEQ(a2.get_field(), 2)
    arktest.assertEQ(a2.my_field, a2.get_field())
}
