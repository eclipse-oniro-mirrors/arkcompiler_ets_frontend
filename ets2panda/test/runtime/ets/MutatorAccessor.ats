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
  _bfield: boolean = true;

  constructor() { }

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

  let get_field = obj.field;
  arktest.assertEQ(get_field, 30)

  get_field = obj.field + obj.field;
  arktest.assertEQ(get_field, 60)

  get_field = obj.field + 10;
  arktest.assertEQ(get_field, 40)

  get_field = 10 + obj.field;
  arktest.assertEQ(get_field, 40)

  obj.field = 10;
  get_field = obj.field;
  arktest.assertEQ(get_field, 10)

  obj.field = obj.field;
  get_field = obj.field;
  arktest.assertEQ(get_field, 10)

  obj.field = obj.field + obj.field;
  get_field = obj.field;
  arktest.assertEQ(get_field, 20)

  obj.field = obj.field + 10;
  get_field = obj.field;
  arktest.assertEQ(get_field, 30)

  obj.field = 10 + obj.field;
  get_field = obj.field;
  arktest.assertEQ(get_field, 40)

  obj.field = 10 * obj.field;
  get_field = obj.field;
  arktest.assertEQ(get_field, 400)

  obj.field = obj.field / 2;
  get_field = obj.field;
  arktest.assertEQ(get_field, 200)

  obj.field = 1;
  obj.field = obj.field & 0;
  get_field = obj.field;
  arktest.assertEQ(get_field, 0)

  obj._bfield = true;
  obj._bfield = obj._bfield & false;
  let get_bfield = obj._bfield;
  arktest.assertEQ(get_bfield, false)

  obj._bfield = true;
  obj._bfield = obj._bfield | false;
  get_bfield = obj._bfield;
  arktest.assertEQ(get_bfield, true)

  obj._bfield = true;
  obj._bfield = obj._bfield | obj._bfield;
  get_bfield = obj._bfield;
  arktest.assertEQ(get_bfield, true)
}
