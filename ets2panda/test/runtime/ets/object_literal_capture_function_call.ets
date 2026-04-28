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

/*---
desc: Object literal callbacks call captured function-typed variables
tags: []
---*/

interface XmlHandler {
    endDocument(): void
}

interface CallbackRunner {
    run(): void
}

function parseXmlAsync(): Promise<int> {
    return new Promise<int>((resolve, reject) => {
        const handler: XmlHandler = {
            endDocument: () => {
                resolve(1)
            }
        }

        handler.endDocument()
    })
}

function createNoArgRunner(callback: () => void): CallbackRunner {
    return {
        run: () => {
            callback()
        }
    }
}

function createOptionalRunner(callback: (value?: int) => void): CallbackRunner {
    return {
        run: () => {
            callback()
        }
    }
}

function createRestRunner(callback: (...values: int[]) => void): CallbackRunner {
    return {
        run: () => {
            callback(1, 2, 3)
        }
    }
}

function testNoArgCallback(): int {
    let noArgCount = 0
    const runner = createNoArgRunner(() => {
        noArgCount += 1
    })

    runner.run()
    runner.run()
    return noArgCount
}

function testOptionalParamCallback(): int {
    let callCount = 0
    const runner = createOptionalRunner((value?: int): void => {
        callCount += 1
    })

    runner.run()
    return callCount
}

function testRestParamCallback(): int {
    let argumentCount = 0
    const runner = createRestRunner((...values: int[]): void => {
        argumentCount = values.length
    })

    runner.run()
    return argumentCount
}

async function main(): Promise<void> {
    const promiseResult = await parseXmlAsync()
    arktest.assertEQ(promiseResult, 1)

    arktest.assertEQ(testNoArgCallback(), 2)
    arktest.assertEQ(testOptionalParamCallback(), 1)
    arktest.assertEQ(testRestParamCallback(), 3)
}
