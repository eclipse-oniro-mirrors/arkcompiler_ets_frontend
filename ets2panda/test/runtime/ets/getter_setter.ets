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
    private _field: int = 30;
    private _bfield: boolean = true;

    get field(): int {
        return this._field;
    }

    set field(a: int) {
        this._field = a;
    }

    get bfield(): boolean {
        return this._bfield;
    }

    set bfield(b: boolean) {
        this._bfield = b;
    }
}

function main(): void {
    let obj: A = new A();

    let h = obj.field;
    arktest.assertEQ(h, 30)
    let z = obj.field + obj.field;
    arktest.assertEQ(z, 60)
    let u = obj.field + 10;
    arktest.assertEQ(u, 40)
    let i = 10 + obj.field;
    arktest.assertEQ(i, 40)

    obj.field = 10;
    let k = obj.field;
    arktest.assertEQ(k, 10)

    obj.field = obj.field;
    k = obj.field;
    arktest.assertEQ(k, 10)

    obj.field = obj.field + obj.field;
    k = obj.field;
    arktest.assertEQ(obj.field, 20)

    obj.field = obj.field + 10;
    k = obj.field;
    arktest.assertEQ(k, 30)

    obj.field = 10 + obj.field;
    k = obj.field;
    arktest.assertEQ(k, 40)

    obj.field = 10 * obj.field;
    k = obj.field;
    arktest.assertEQ(k, 400)

    obj.field = obj.field / 2;
    k = obj.field;
    arktest.assertEQ(k, 200)

    obj.field = 1;
    obj.field = obj.field & 0;
    k = obj.field;
    arktest.assertEQ(k, 0)

    obj.bfield = true;
    obj.bfield = obj.bfield & false;
    let boolk = obj.bfield;
    arktest.assertEQ(boolk, false)

    obj.bfield = true;
    obj.bfield = obj.bfield | false;
    boolk = obj.bfield;
    arktest.assertEQ(boolk, true)

    obj.bfield = true;
    obj.bfield = obj.bfield | obj.bfield;
    boolk = obj.bfield;
    arktest.assertEQ(boolk, true)
}
