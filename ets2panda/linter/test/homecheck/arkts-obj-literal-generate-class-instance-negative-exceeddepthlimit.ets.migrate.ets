/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
class B {
  v = 2
}

function test1() {
  let obj: B = { v: 100 }
  return obj;
}

function test2() {
  let obj = test1()
  return obj;
}

function test3() {
  let obj = test2()
  return obj;
}

function test4() {
  let obj2 = test3();
  let _ = obj2 instanceof B;
}

function test5() {
  let obj: B = {v: 2};
  test6(obj);
}

function test6(obj: B) {
  test7(obj);
}

function test7(obj: B) {
  test8(obj);
}

function test8(obj: B) {
  let _ = obj instanceof B;
}

@Entry
@Component
struct Index {
  build() {
  }
}