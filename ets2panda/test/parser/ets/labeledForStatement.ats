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

function labeledForTest01(): void {
  label1:
  for (let i = 0; i < 10; ++i) {
    if (i > 0) {
      continue label1;
    } else {
      break label1;
    }
  }

  label2:
  for (let i = 0; i < 10; ++i) {
    for (let j = 0; j < 10; ++j) {
      if (j > 0 || i > 1) {
        continue label2;
      } else {
        break label2;
      }
    }
  }
}
