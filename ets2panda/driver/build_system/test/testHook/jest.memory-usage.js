/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

let startMem = 0;

beforeEach(() => {
  global.gc?.();
  startMem = process.memoryUsage().heapUsed;
});

afterEach(() => {
  const endMem = process.memoryUsage().heapUsed;
  const peak = (endMem - startMem) / 1024 / 1024;
  const testName = expect.getState().currentTestName;
  console.log(`[Jest][${testName} used ${peak.toFixed(2)} MB]`);
});
