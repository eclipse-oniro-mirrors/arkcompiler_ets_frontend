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

class TestError extends Error {}

let catchCode = 0;

function main(): void {
  try {
    throw new DivideByZeroError();
  } catch (p: DivideByZeroError) {
    catchCode = 1;
  } catch (p: Error) {
    arktest.assertTrue(false)
  }

  arktest.assertEQ(catchCode, 1)

  try {
    throw new DivideByZeroError();
  } catch (p: Error) {
    catchCode = 2;
  } catch (p: DivideByZeroError) {
    arktest.assertTrue(false)
  }

  arktest.assertEQ(catchCode, 2)

  try {
    throw new TestError();
  } catch (p: TestError) {
    catchCode = 3;
  } catch (p: Error) {
    arktest.assertTrue(false)
  }

  arktest.assertEQ(catchCode, 3)

  try {
    throw new TestError();
  } catch (p: Error) {
    catchCode = 4;
  } catch (p: TestError) {
    arktest.assertTrue(false)
  }

  arktest.assertEQ(catchCode, 4)
}
