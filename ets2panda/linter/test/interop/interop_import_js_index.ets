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
import { ff3 } from "./interop_import_js_rules_js"
import {foo} from "./interop_import_js_index_js"
let arr = foo.arr
arr[1]
arr[3] = 4

let arr1 = ff3.arr
let len = arr1.length as number
for (let i = 0; i < arr1.length; ++i) {
    console.log(arr1[i]+'');  //error
    let x = arr1[i]  //error
    arr1[i] = 0  //error
    console.log(arr1[i]+'');  //error
}

for (let element of arr1) {  //error
  if (element == 8) {
    console.log("hi");
  }
}
