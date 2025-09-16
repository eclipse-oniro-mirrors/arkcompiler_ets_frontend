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

function fors(): void {
  for (let i = 1; i < 5; i++) continue;
  let j = 1;
  for (j = 1; i < 5; i++) continue;
  // Valid but must be fixed in grammar
  // for (;;) break;
  for (let i = 1, j = 3; i + j < 30; i += j) {
    continue;
  }
}

function whiles(): void {
  while (false) {
  }

  while (true) break;
}

function dowhiles(): void {
  let i = 0;
  do
    i += 1;
  while (i < 30);

  do {
    i *= -1
  } while (i != 30);
}

// see 6.9
function labeledbreak(): void {
  loop1:
  for (let i = 1; i < 5; i++) {
    loop2:
    for (let j = 1; j < 5; j++) {
      break loop1;
    }
  }
}

// see 6.10
function labeledcontinue(): void {
  loop1:
  for (let i = 1; i < 5; i++) {
    loop2:
    for (let j = 1; j < 5; j++) {
      continue loop1;
    }
  }
}

/* @@? 19:15 Error TypeError: Unresolved reference i */
