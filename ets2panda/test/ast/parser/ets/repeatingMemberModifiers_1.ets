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

class Base {
  readonly field: number = 0
}
class A extends Base {
  override readonly /* @@ label */override field: number = 5
}

function main(): void {
  let a: A = new A()
  arktest.assertTrue(a.field instanceof number)
  arktest.assertEQ(a.field, 5)
}

/* @@@ label Error SyntaxError: Duplicated modifier is not allowed. */