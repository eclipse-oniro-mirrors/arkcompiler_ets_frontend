/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

async function asyncFoo(): Promise<Object | null> {
    let promise: Promise<Object | null> | null = (): Promise<Object | null> | null => { return null; }();
    let obj: Object | null = await promise!;
    return promise;
}

let asyncLambda: () => Promise<Object | null> = async (): Promise<Object | null> => {
    let promise: Promise<Object | null> | null = (): Promise<Object | null> | null => { return null; }();
    let obj: Object | null = await promise!;
    return promise;
}

async function foo(): Promise<void> {
    let promise: Promise<Object | null> | null = (): Promise<Object | null> | null => { return null; }();
    let obj: Object | null = await promise!;
}

let lambda: () => Promise<void> = async (): Promise<void> => {
    let promise: Promise<Object | null> | null = (): Promise<Object | null> | null => { return null; }();
    let obj: Object | null = await promise!;
}

let promise: Promise<Object | null> | null = null;
let obj: Object | null;
waitForCompletion(async () => {
    obj = await promise!;
})
