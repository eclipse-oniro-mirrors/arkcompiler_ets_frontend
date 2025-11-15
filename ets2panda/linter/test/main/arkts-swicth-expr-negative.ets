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

const isTrue = true;
switch (isTrue) {
  case true:
    console.log('It\'s true');
    break;
  case false:
    console.log('It\'s false')
    break;
}

let objValue = { key: "value" };

switch (objValue) {
  case { key: "value" }:
    console.log("Matching object!");
    break;
  default:
    console.log("Non-matching object");
}

let arrayValue = [1, 2, 3];

switch (arrayValue) {
  case [1, 2, 3]:
    console.log("Matching array!");
    break;
  default:
    console.log("Non-matching array");
}

type a = number | string | Function
let aa: a = ()=>{};
switch (aa) { // err
  case Function:
    break;
}