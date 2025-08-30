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

let res: string = "";
final class AA {
  getString(arg: number) { res = "instance" }
  static getString(arg: number) { res = "static"; }
}

function foo() {
  let a = new AA()
  let radix: number = 16
  a.getString(radix)
}

foo();
arktest.assertTrue(res == "instance");

