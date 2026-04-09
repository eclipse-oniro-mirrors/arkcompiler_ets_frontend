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


let keyof: string[] = ["a", "b", "c"]
for (let i: number = 0; i < keyof.length; i++) {
}

function keyof_func(keyof: string): string {
    return keyof
}

let obj = {
    keyof: "value"
}

let arr: string[] = ["x", "y", "z"]
for (let j = 0; j < arr.length; j++) {
    let keyof = arr[j]
}

if (keyof.length > 0) {
    keyof.pop()
}

while (keyof.length < 5) {
    keyof.push("new")
}
