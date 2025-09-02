/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

function main(): void {
  let a : int = 0;
  let b : int = 0;

  for (let aa : int = 1, bb : int = 1; aa<5 && bb<5; aa++, bb++) {
    a += aa;
    b += bb;
  }
  arktest.assertTrue(a == 10 && b == 10)

  let aa : int = 10;
  let bb : int = 10;
  for (aa = 1, bb = 1; aa<5 && bb<5; aa++, bb++) {
    a += aa;
    b += bb;
  }
  arktest.assertTrue(a == 20 && b == 20)
}
