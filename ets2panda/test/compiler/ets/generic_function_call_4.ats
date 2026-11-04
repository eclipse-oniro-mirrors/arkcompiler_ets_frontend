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

class A {}
class B<T> {}

function test2<T>(): B<T> {
  let tmp: B<T> = new B<T>();
  return tmp;
}

function test3<T>(): B<B<T>> {
  let tmp: B<B<T>> = new B<B<T>>();
  return tmp;
}

function main(): void {
  let bClass: B<A> = test2<A>();
  let bClass2: B<B<A>> = test3<A>();
}
