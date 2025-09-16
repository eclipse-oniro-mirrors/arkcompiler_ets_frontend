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

function fn(o: Object): Object {
  let arr = o as long[];
  arktest.assertEQ(arr[5], 42)
  arr[6] = arr[5] * 2;
  return arr as Object;
}

function main() {
  let arr = new long[10];
  arr[5] = 42;

  let obj = fn(arr as Object);
  arktest.assertEQ((obj as long[])[6], 84)
}
