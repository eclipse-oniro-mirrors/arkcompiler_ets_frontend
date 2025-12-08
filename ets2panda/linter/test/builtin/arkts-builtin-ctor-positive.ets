/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

// P1: Direct Instantiation
function createAggregateError() {
  return AggregateError([Error("first"), Error("second")]);
}

// P2: Direct Functional Call 
function createEmptyArray() {
  return Array<string>(0);
}

// P3: Direct Instantiation
function createBigIntDirect() {
  return BigInt(1);
}

// P4: Conditional Check/Loop Iteration 
function findError(items: Error[]) {
  for (const item of items) {
    if (item && item.name === 'AggregateError') {
      return item;
    }
  }
  return null;
}

// P5: Exact Generic Instantiation/Array Element
const allowedIdentifiers: Array<string> = [
  "Error",
  "AggregateError",
];