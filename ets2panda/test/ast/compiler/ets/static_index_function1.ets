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
  static $_get(i : string) : number {
      return 42.0
  }
}

function main() {
    let a = new A()
    let b = a.$_get('a')
    let c = A.$_get('a')
    let d = a['a']
    let e = A['a']
}


/* @@? 24:15 Error Semantic error ESE0207: '$_get' is a static property of 'A' */
/* @@? 26:15 Error Semantic error ESE0207: '$_get' is a static property of 'A' */
/* @@? 27:13 Error Syntax error ESY0318: Class cannot be used as object. */
/* @@? 27:15 Error Semantic error ESE0207: '$_get' is a static property of 'A' */