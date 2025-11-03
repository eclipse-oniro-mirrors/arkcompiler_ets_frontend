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


export function atomicTask() {
    const buffer = new ArrayBuffer(4);
    const sharedArray = new Int32Array()
    for (let i = 0; i < 10; i++) {
    Atomics.add(shareArray, 0, 1);
    }
    return Atomics.load(shareArray, 0);
}

/* @@? 21:5 Error TypeError: No matching call signature for add(shareArray, Int, Int) */