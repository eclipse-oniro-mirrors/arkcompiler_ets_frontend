/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

function foo(arg: number|undefined) {
    arg!++;
    callMe();
    return arg;
}

function main(): void {
    let a = foo(1);
    assertEQ(a, 2.0)
}

class Test {
    n: int|undefined = 1
}

function callMe():int{
  let v1 : int[] = [1,2,3];
  let b = new Test();
  b.n!++;
  v1[0]!++;
  return v1[0];
}