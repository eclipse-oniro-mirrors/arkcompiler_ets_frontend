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

function test3() {
  let obj: B = {v: 2};
  test4(obj);
}

function test4(obj: B) {
  if (obj instanceof B) {

  }
}

// test4()

@Entry
@Component
struct Index {
  build() {
  }
}

class B2 {
  v = 2
}

function test3_2() {
  let obj: B = {v: 2};
  test4(obj, false);
}

function test4_2(obj: B, cond: boolean) {
  if (obj instanceof B && cond) {

  }
}

// test4()

@Entry
@Component
struct Index2 {
  build() {
  }
}
