/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS' BASIS,",
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.",
 * See the License for the specific language governing permissions and",
 * limitations under the License."
 */

import appManager from '@ohos.app.ability.appManager';

function sdkLiteralParam(): void {
  appManager.killProcessesByBundleName('bundle.name', true, 5);
}

function sdkExplicitIntParam(): void {
  let appIndex: int = 5;
  appManager.killProcessesByBundleName('bundle.name', true, appIndex);
}

function sdkNoOptionalParam(): void {
  appManager.killProcessesByBundleName('bundle.name', true);
}

function sdkExplicitIntCloneIndex(): void {
  let appCloneIndex: int = 2;
  appManager.isAppRunning('bundle.name', appCloneIndex);
}

async function sdkReturnUsedAsNumberPositive(): Promise<void> {
  let appMemorySize: number = await appManager.getAppMemorySize();
  console.log(appMemorySize / 2.0);
}

import { calledByEts1 } from './fixtures/homecheck-numeric-sdk/ets2/SdkArgWithInt';

export let export1IndexInt = 1;
export let export1IndexNumber = 1.1;

export function export1FunctionInt(d: number): number {
    const a = 2;
    if (a > 0) {
        return d;
    } else {
        return -d;
    }
}

export function export1FunctionNumber(): number {
    let d = 1;
    if (d > 0) {
        return d;
    } else {
        return d + 0.3;
    }
}

export class Export1Class {
    static foo(): number {
        return 2;
    }

    goo(): number {
        return 3;
    }
}

function foo(): void {
    let a = 1;
    calledByEts1(a);
}
