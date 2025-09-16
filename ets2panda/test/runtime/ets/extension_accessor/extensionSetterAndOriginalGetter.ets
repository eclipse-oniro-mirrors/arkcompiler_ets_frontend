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

class A {
    _field: int = 30;
    static _sfield: int = 30;
    get field(): int {
        return this._field;
    }

    get sfield(): int {
        return A._sfield;
    }
}

set field(this: A, f: int) {
    this._field = f;
}

set sfield(this: A, f: int) {
    A._sfield = f;
}

function main(): void {
    let obj: A = new A();
    arktest.assertEQ(obj.field, 30);
    obj.field = 1;
    arktest.assertEQ(obj.field, 1);
    obj.field++;
    arktest.assertEQ(obj.field, 2);
    ++obj.field;
    arktest.assertEQ(obj.field, 3);
    new A().sfield = 30;
    arktest.assertEQ(new A().sfield, 30);
    new A().sfield = 1;
    arktest.assertEQ(new A().sfield, 1);
    new A().sfield++;
    arktest.assertEQ(new A().sfield, 2);
    ++new A().sfield;
    arktest.assertEQ(new A().sfield, 3);
}
