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

function main() {
  let x1 = (): int | undefined => { return 5; }();
  let y1: int = x1 ?? Double.toInt(3.1);
  arktest.assertEQ(y1, 5)

  let x2 = (): int | undefined => { return undefined; }();
  let y2: int = x2 ?? Double.toInt(3.1);
  arktest.assertEQ(y2, 3)

  let x3 = (): number | undefined => { return 5; }();
  let y3: number = x3 ?? 3;
  arktest.assertEQ(y3, 5.0)

  let x4 = (): number | undefined => { return undefined; }();
  let y4: number = x4 ?? 3;
  arktest.assertEQ(y4, 3.0)
}
