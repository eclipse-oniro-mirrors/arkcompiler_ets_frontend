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

function foo() : Array<number> {
    return [1,2,3];
}

function bar() : [number, string] {
    return [42, "abc"];
}

function main() {
  let a: number, b: number, c: number, d: number, str: string;
  [a, b, c] = foo();
  arktest.assertEQ(a, 1);
  arktest.assertEQ(b, 2);
  arktest.assertEQ(c, 3);

  [d, str] = bar();
  arktest.assertEQ(d, 42);
  arktest.assertEQ(str, "abc");
}
