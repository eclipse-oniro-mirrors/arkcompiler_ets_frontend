/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

import appManager from '@ohos.app.ability.appManager';

function sdkAnnotatedIntParam(): void {
  let appIndex: number = 5; // sdk-api-num2int
  appManager.killProcessesByBundleName('bundle.name', true, appIndex);
}

function sdkCopiedIntParam(): void {
  let baseIndex = 1; // sdk-api-num2int
  let copiedIndex = baseIndex; // sdk-api-num2int
  appManager.killProcessesByBundleName('bundle.name', true, copiedIndex);
}

function sdkExpressionParam(): void {
  let expressionIndex = 1 + 2; // sdk-api-num2int
  appManager.killProcessesByBundleName('bundle.name', true, expressionIndex);
}

function sdkDecimalParam(): void {
  let dynamicAppIndex: number = 5.5;
  appManager.killProcessesByBundleName('bundle.name', true, dynamicAppIndex); // sdk-api-num2int
}

function sdkOptionalCloneIndex(): void {
  let appCloneIndex: number = 2; // sdk-api-num2int
  appManager.isAppRunning('bundle.name', appCloneIndex);
}

function sdkOptionalCloneIndexWithDecimal(): void {
  let appCloneIndex: number = 2.5;
  appManager.isAppRunning('bundle.name', appCloneIndex); // sdk-api-num2int
}

async function sdkReturnAsIntParam(): Promise<void> {
  let appMemorySize = await appManager.getAppMemorySize();
  appManager.killProcessesByBundleName('bundle.name', true, appMemorySize); // sdk-api-num2int
}

async function sdkReturnUsedAsNumber(): Promise<void> {
  let appMemorySize = await appManager.getAppMemorySize(); // sdk-api-num2int
  let halfSize = appMemorySize / 2; // sdk-api-num2int
  console.log(halfSize);
}
