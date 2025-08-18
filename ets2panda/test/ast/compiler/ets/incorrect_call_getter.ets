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


interface inter {
    gett(k: number): number;
}

class Getter {
    $_get(index:number) {}
}

export class ConM implements inter {
    private buckets: Getter = new Getter();

    private getBucket(k: int) {
        this.buckets[this.gett](k)
    }
}

/* @@? 22:10 Error TypeError: 'The special predefined method '$_get' shouldn't have void return type. */
/* @@? 25:36 Error TypeError: ConM is not abstract and does not override abstract method gett(k: Double): Double in inter */
/* @@? 29:9 Error TypeError: No matching indexing signature for $_get((k: Double) => Double) */
/* @@? 29:22 Error TypeError: Type '(k: Double) => Double' is not compatible with type 'Double' at index 1 */
/* @@? 29:22 Error TypeError: Cannot find index access method with the required signature. */
