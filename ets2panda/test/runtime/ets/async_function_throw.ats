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

async function asyncFuncAlwaysThrow1(): Promise<string> {
    throw new Error("Something went wrong!");
}

async function asyncFuncAlwaysThrow2(): Promise<int> {
    throw new Error("Something went wrong!");
}

const asyncLambdaAlwaysThrow1 = async (): Promise<string> => {
    throw new Error("Something went wrong!");
}

const asyncLambdaAlwaysThrow2 = async (): Promise<int> => {
    throw new Error("Something went wrong!");
}

async function asyncFuncThrow1(cond: boolean): Promise<string> {
    if (cond) {
        return "str"
    } else {
        throw new Error("Something went wrong!");
    }
}

async function asyncFuncThrow2(cond: boolean): Promise<string> {
    if (cond) {
        return "str"
    }
    throw new Error("Something went wrong!");
}

async function startTest() {
    try {
        await asyncFuncAlwaysThrow1();
    } catch (error) {
        arktest.assertEQ(error.message, "Something went wrong!");
    }

    try {
        await asyncFuncAlwaysThrow2();
    } catch (error) {
        arktest.assertEQ(error.message, "Something went wrong!");
    }

    try {
        await asyncLambdaAlwaysThrow1();
    } catch (error) {
        arktest.assertEQ(error.message, "Something went wrong!");
    }

    try {
        await asyncLambdaAlwaysThrow2();
    } catch (error) {
        arktest.assertEQ(error.message, "Something went wrong!");
    }

    try {
        await asyncFuncThrow1(false);
    } catch (error) {
        arktest.assertEQ(error.message, "Something went wrong!");
    }

    try {
        await asyncFuncThrow2(false);
    } catch (error) {
        arktest.assertEQ(error.message, "Something went wrong!");
    }

    await asyncFuncThrow1(true);
    await asyncFuncThrow2(true);
}

function main(): void {
    waitForCompletion(startTest);
}
