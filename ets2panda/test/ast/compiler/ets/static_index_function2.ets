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

class A {
  static $_set(i: string, v: number) {
      return
  }
}

function main(): int {
  let a = new A
  a[/* @@ label */'a'] = 42.0
  /* @@ label1 */A[/* @@ label2 */'a'] = 42.0
  return 0
}


/* @@@ label Error Semantic error ESE0207: '$_set' is a static property of 'A' */
/* @@@ label1 Error Syntax error ESY0318: Class cannot be used as object. */
/* @@@ label2 Error Semantic error ESE0207: '$_set' is a static property of 'A' */