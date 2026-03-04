/**
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

// Test that non-promise objects can be used with await
// The key test is when the type is a union of Promise<T> | T

// Function that may return Promise or value
function maybePromise(): Promise<string> | number {
    return 42;
}
    
async function testAwaitUnionType(): Promise<void> {
    // This should work: await handles both Promise and non-Promise
    let result: number|string = await maybePromise();
    arktest.assertEQ(result, 42);
}

// Function that may return Promise or undefined
function maybePromiseOrUndefined(): Promise<string> | undefined {
    return undefined;
}

 async function testAwaitUnionWithUndefined(): Promise<void> {
    let result: string | undefined = await maybePromiseOrUndefined();
    arktest.assertEQ(result, undefined);
}

async function testAwaitConditionalPromise(): Promise<void> {
    // Conditional that may be Promise or non-Promise
    let condition: boolean = true;
    let value: Promise<number> | number = condition ? Promise.resolve(100.0) : 200;
    
    let result: number = await value;
    arktest.assertEQ(result, 100);
}

async function testAwaitNonPromiseDirect(): Promise<void> {
    // Direct non-promise await (should just return the value)
    let num: number = await 42;
    arktest.assertEQ(num, 42);

    let str: string = await "hello";
    arktest.assertEQ(str, "hello");
}

function main(): void {
    waitForCompletion(async () => {
        await testAwaitUnionType();
        await testAwaitUnionWithUndefined();
        await testAwaitConditionalPromise();
        await testAwaitNonPromiseDirect();
    });
}
