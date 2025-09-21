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
  [{...a}= b];
}

function test0() {
  let v1 = "v1";
  let v2 = "v2";
  let v3 = {"key": "value"};
  [{"a": v1, "b": v2, ...c = v3}];
}
function test1() {
  let v3 = {"key": "value"};
  [{"a": v1, "b": {"c": v2, ...d = v3}}];
}

function test2() {
  let key = "b";
  [{[key]: v2}];
}

function test3() {
  [{a, b}];
}

function test4() {
  [{...c}];
}

function test5() {
  [{...c, "a": v1}];
}

function test6() {
  [{"a": {"b": {...d}}}];
}