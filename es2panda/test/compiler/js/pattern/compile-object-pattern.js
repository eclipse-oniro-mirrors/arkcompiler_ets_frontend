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

function test() {
  let a;
  let b = {"key": "success"};
  [{...a} = b];
  print(a["key"]);
}
test();

function test0() {
  let v1 = "first";
  let v2 = "second";
  let v3 = {"key": "success"};
  let c;
  [{"a": v1, "b": v2, ...c = v3}] 
  print(c["key"]);
}
test0();

function test1() {
  let v1 = "first";
  let v2 = "second";
  let v3 = {"key": "success"};
  let d;
  [{"a": v1, "b": {"c": v2, ...d = v3}}];
  print(d["key"]);
}
test1();