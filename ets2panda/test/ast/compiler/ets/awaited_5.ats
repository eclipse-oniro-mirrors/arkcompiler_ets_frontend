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

type T1 = Awaited<Promise<string>>
type T2 = Awaited<PromiseLike<string>>

async function main() {
    let v1: T1 = "1"
    let v2: T2 = Promise.resolve("1")

    arktest.assertTrue(v1 instanceof string)
    arktest.assertTrue(v2 instanceof PromiseLike)
}

/* @@? 23:24 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
/* @@? 24:24 Warning Warning W1001506: the value of the instanceof expression is known at compile-time as true */
