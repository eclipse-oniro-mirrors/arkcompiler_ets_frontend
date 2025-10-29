/*
 * Copyright (c) 2025 - 2025 Huawei Device Co., Ltd.
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

let a: FixedArray<int> = [1, 2, 3];

function forins(): string {
  for (let i: string of "abcdef") {
      return i;
  }
  return '*';
}

function main(): void {
  let res = 0;
  for (let v: double of a) {
    res += v;
  }
  /* @@ label */for (let v: float of [4.1, 5.4, 6.0]) {
    res += v;
  }
}


/* @@@ label Error Semantic error ESE0069: Source element type 'Double' is not assignable to the loop iterator type 'Float'. */