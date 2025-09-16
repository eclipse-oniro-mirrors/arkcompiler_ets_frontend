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


function foo(str: string | undefined): undefined {
  let x: string | undefined = undefined;
  for (let i = 0; i < 1; ++i) {
    x = str;
    if (x == "a") {
      break;
    } else {
      x = undefined;
    }
  }
  return /* @@ label */x;
}


function main() {
  console.log(foo("a"));
}

/* @@@ label Error TypeError: Type 'String|undefined' is not compatible with the enclosing method's return type 'undefined' */
