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

interface RA<T> {
  foo(c: T, a?: Number): number | undefined
}
class A<T> implements RA<T> {
  foo(c: T, a?: Number): number | undefined {
    return a;
  }
}

function main(): void {
    let a = new A<Number>();
    assertEQ(a.foo(1, 2), 2)
    assertEQ(a.foo(1), undefined)
}
