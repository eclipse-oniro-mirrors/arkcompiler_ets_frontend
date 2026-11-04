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

class A {
    _field: int = 30;

    constructor() { }

    get field(): int {
        return this._field;
    }
}

class B {
    _field: int = 30;

    constructor() { }

    set field(f:int) {
        this._field = f;
    }
}

function main(): void {
    let obj: A = new A();

    obj._field = 100;
    let get_field = obj.field;
    arktest.assertEQ(get_field, 100)

    let b_obj: B = new B();

    b_obj.field = 100;
    let get_b_field = b_obj._field;
    arktest.assertEQ(get_b_field, 100)
}
