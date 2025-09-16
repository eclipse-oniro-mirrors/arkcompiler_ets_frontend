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

function foo(arg: Object) : int {
  return 1;
}

function fooB(arg: Object|undefined) : int {
  return 2;
}

function fooC(arg: Object|undefined|null) : int {
  return 3;
}

function fooD(arg: undefined|null|int) : int {
  return 4;
}

function fooE(arg: undefined|null|boolean) : int {
  return 5;
}

function fooF(arg: undefined|null|string) : int {
  return 6;
}

function fooG(arg: undefined|null|string) : int {
  return 7;
}


function main(): void {
  let a: Object|undefined = new Object();
  let b: Object|undefined = undefined;
  let c: Object|undefined|null = null;
  let d: int|undefined|null = 0;
  let e: boolean|undefined|null = false;
  let f: string|undefined|null = "";
  let g: string|undefined|null = "Hello World!";

  arktest.assertEQ( foo(a), 1 );
  arktest.assertEQ( foo(a!), 1 );
  arktest.assertEQ( fooB(b), 2 );
  arktest.assertEQ( fooC(c), 3 );
  arktest.assertEQ( fooD(d!), 4 );
  arktest.assertEQ( fooE(e!), 5 );
  arktest.assertEQ( fooF(f!), 6 );
  arktest.assertEQ( fooG(g!), 7 );
}