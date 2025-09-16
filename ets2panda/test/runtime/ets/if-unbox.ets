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

function cond1(): int {
  let a = 1 as Int;
  if(a != 5) return 1;
  return 0;
}

function cond2(): int {
  if((1 as Int) != 5) return 1;
  return 0;
}

function main(): void {
  arktest.assertEQ(cond1(), 1)
  arktest.assertEQ(cond2(), 1)
}
