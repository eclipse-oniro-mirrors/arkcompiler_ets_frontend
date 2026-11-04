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

class cls {
  member_cls: cls | null;
  member_int: int;

  constructor(arg_cls: cls | null, arg_int: int) {
    this.member_cls = arg_cls;
    this.member_int = arg_int;
  }
}

class cls2 {
  member_cls: cls | null = null;
  member_int: int;
}

function main(): void {
    // TODO: catch NullPointerError if exception handling is implemented
    // let x : cls;
    // x.member_cls;

    let y : cls = new cls(null, 2);
    arktest.assertTrue(y.member_cls == null && y.member_int == 2)

    // TODO: catch NullPointerError if exception handling is implemented
    // y = null;
    // y.member_cls;

    let z : cls = new cls(y, 4);
    arktest.assertTrue(z.member_cls != null && z.member_int == 4)

    let u : cls2 = new cls2();
    arktest.assertTrue(u.member_cls == null && u.member_int == 0)

    return;
}
