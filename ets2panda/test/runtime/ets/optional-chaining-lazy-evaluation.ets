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

function foo1(): Int[] | null {
    return null;
}

function foo2(): Int[] | null {
    let obj_tmp: Int[] = [11, 21, 31];
    return obj_tmp;
}

function main(): void {
  let x: int = 0;

  let potentiallyNullObj: Int[] | null = foo1();
  let numb: Int | undefined = potentiallyNullObj?.[x++] ?? 2;
  arktest.assertEQ(numb, 2)
  arktest.assertEQ(x, 0) // 0 as x was not incremented

  let obj: Int[] | null = foo2();
  numb = obj?.[x++];

  let a : Int = 11;
  arktest.assertEQ(numb, a)
  arktest.assertEQ(x, 1) // 1 as x was incremented
}
