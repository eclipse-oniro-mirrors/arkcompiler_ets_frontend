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

class TestException extends Exception {}

let catchCode = 0;

function main(): void {
  try {
    throw new NullPointerError();
  } catch (e: NullPointerError) {
    catchCode = 1;
  } catch (e: Error) {
    arktest.assertTrue(false)
  } catch (e) {
    arktest.assertTrue(false)
  }

  arktest.assertEQ(catchCode, 1)

  try {
    throw new NullPointerError();
  } catch (e: Error) {
    catchCode = 2;
  } catch (e: NullPointerError) {
    arktest.assertTrue(false)
  } catch (e) {
    arktest.assertTrue(false)
  }

  arktest.assertEQ(catchCode, 2)

  try {
    throw new TestException();
  } catch(e: TestException) {
    catchCode = 3;
  } catch (e: Exception) {
    arktest.assertTrue(false)
  } catch (e) {
    arktest.assertTrue(false)
  }

  arktest.assertEQ(catchCode, 3)

  try {
    throw new TestException();
  } catch (e: Exception) {
    catchCode = 4;
  } catch (e: TestException) {
    arktest.assertTrue(false)
  } catch (e) {
    arktest.assertTrue(false)
  }

  arktest.assertEQ(catchCode, 4)
}
