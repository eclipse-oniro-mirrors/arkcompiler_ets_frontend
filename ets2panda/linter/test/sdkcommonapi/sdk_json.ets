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
import json from './api/@ohos.util.json'

let obj = json.parse(''); //error
function aa(this: Object, key: string, value: Object): Object | undefined | null {
  return null;
}
let obj1 = json.parse('', aa); //error
json.remove(obj, ''); //error
const uppercaseTransformer: json.Transformer = (key, value) => { //error
  if (typeof value === 'string') {
    return value.toUpperCase();
  }
  return value;
};
//sum:4