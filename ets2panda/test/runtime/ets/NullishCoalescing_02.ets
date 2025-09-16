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

function foo1(z: long|undefined) {
  let x = z ?? 10000000000;
  return x;
}

function foo2(z: int|undefined) {
  let x = z ?? 10000000;
  return x;
}

function foo3(z: short|undefined) {
  let x = z ?? 10000;
  return x;
}

function foo4(z: byte|undefined) {
  let x = z ?? 100;
  return x;
}

function foo5(z: float|undefined) {
  let x = z ?? 100000;
  return x;
}

function foo6(z: float|undefined) {
  let x = z ?? 100000.0f;
  return x;
}

function foo7(z: double|undefined) {
  let x = z ?? 10000000;
  return x;
}

function foo8(z: double|undefined) {
  let x = z ?? 10000000.0f;
  return x;
}

function foo9(z: double|undefined) {
  let x = z ?? 10000000.0;
  return x;
}


function main() {
  arktest.assertEQ(foo1(undefined), 10000000000);
  arktest.assertEQ(foo2(undefined), 10000000);
  arktest.assertEQ(foo3(undefined), 10000);
  arktest.assertEQ(foo4(undefined), 100);
  arktest.assertEQ(foo5(undefined), 100000.0f);
  arktest.assertEQ(foo6(undefined), 100000.0f);
  arktest.assertEQ(foo7(undefined), 10000000.0);
  arktest.assertEQ(foo8(undefined), 10000000.0);
  arktest.assertEQ(foo9(undefined), 10000000.0);
}
