/**
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

function func06(): void {
  EtsStepArkTest();
}

function func05(): void {
  func06();
  EtsStepArkTest();
}

function func04(): void {
  func05();
  EtsStepArkTest();
}

function func03(): void {
  let id = 0;
  for (let i = 0; i < 10; ++i) {
    id++
  }
  func04();
  EtsStepArkTest();
}

function func02(): void {
  func03();
  EtsStepArkTest();
}

function func01(): void {
  func02();
  EtsStepArkTest();
}

function main(argc:string[]): void {
  func01();
}
