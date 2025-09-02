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

class StringArray {
  [/* @@ label1 */index: number]: string
}
function getStringArray(): StringArray {
  return /* @@ label2 */["a", "b", "c"]
}
let myArray: StringArray = getStringArray()
let secondItem = /* @@ label3 */myArray[1]

/* @@@ label1 Error TypeError: Indexed signatures are not allowed. Use arrays instead!  */
/* @@@ label2 Error TypeError: Type 'Array<String>' is not compatible with the enclosing method's return type 'StringArray'  */
/* @@@ label3 Error TypeError: Object type doesn't have proper index access method.  */
