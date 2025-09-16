/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

class A<T> {
  f: T
  constructor(f: T) { this.f = f }
  $_get(index: number): T { return this.f }
}

function main(): void {
  let o_double = new A<number>(10)
  const o_int = new A<int>(10)
  arktest.assertEQ(o_double[11], 10)
  arktest.assertEQ(10, o_int[11])
}
