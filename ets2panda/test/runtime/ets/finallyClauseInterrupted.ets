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

function main() : void{
  let a = 0;
  let b = 0;
  loop:for (let i = 0; i < 10; i++) {
    try {
      for(let j = 0; j < 10; j++) {
        try {
        } catch (e) {
          arktest.assertTrue(false)
        } finally {
          break;
        }
      }
    } catch (e) {
      arktest.assertTrue(false)
    } finally {
      a++;
    }
  }

  arktest.assertEQ(b, 0)
  arktest.assertEQ(a, 10)
}
