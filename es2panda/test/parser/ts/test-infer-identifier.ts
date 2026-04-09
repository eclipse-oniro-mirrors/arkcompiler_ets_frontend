/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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


let infer: number[] = []
for (let i: number = 0; i < infer.length; i++) {
}

function infer_func(infer: number): number {
    return infer + 1
}

let obj = {
    infer: 1
}

let arr: number[] = [1, 2, 3]
for (let j = 0; j < arr.length; j++) {
    let infer = arr[j]
}

if (infer.length > 0) {
    infer.pop()
}

while (infer.length < 10) {
    infer.push(1)
}
