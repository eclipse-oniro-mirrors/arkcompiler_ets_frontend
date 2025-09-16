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

class C {}

function foo(arg?: number): C | undefined {
  if (arg == 5) return new C();
  return arg == 3 ? undefined : new C();
}

function main() {
    arktest.assertTrue(foo(5) instanceof C)
    arktest.assertEQ(foo(3), undefined)
    arktest.assertTrue(foo(undefined) instanceof C)
}
