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

const asyncFunction = async () => {
    return new Promise<number>((resolve, reject) => {
        setTimeout(() => {
            reject(new Error())
        }, 1000)
    })
}

async function main() {
    const result : number = await asyncFunction()
    .then((result) => {
            return new Promise<number>((resolve) => {
                setTimeout(() => {
                    resolve(result + 1)
                }, 500)
            })
        })
    .then((result) => {
            return new Promise<number>((resolve) => {
                setTimeout(() => {
                    resolve(result + 4)
                }, 500)
            })
    }).catch((): number => {
        console.log("PromiseMultiAsyncOperationTest0300 test error")
        return 0
    })
}