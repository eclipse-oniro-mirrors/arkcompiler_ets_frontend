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

class C<out T> {
  readonly fld: T
  constructor(p: T) {
      this.fld = p
  }
}

type NestedC = String | C<NestedC> | Error

function main(): int {
    let x: NestedC = new C<C<Error>>(new C<Error>(new Error()))
    let ok = x instanceof C && x.fld instanceof C && x.fld.fld instanceof Error
    return ok ? 0 : 1
}
