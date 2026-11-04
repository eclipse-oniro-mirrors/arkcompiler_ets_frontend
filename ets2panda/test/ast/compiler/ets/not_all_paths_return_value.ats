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

async function test1(isError: boolean) {
  if (isError) {
    return 1
  }
  // error not all code paths return a value.
}

async function test(isError: boolean): Promise<string> {
  if (isError) {
    return Promise.resolve("undefined")
  }
  // error not all code paths return a value.
}


async function test2(isError: boolean): Promise<undefined> { //ok
  if (isError) {
    return Promise.resolve(undefined)
  }
}

async function test3(isError: boolean): Promise<void> { //ok
  if (isError) {
    return Promise.resolve(undefined)
  }
}



/* @@? 16:16 Error Semantic error ESE0104: Not all code paths return a value. */
/* @@? 23:16 Error Semantic error ESE0104: Not all code paths return a value. */