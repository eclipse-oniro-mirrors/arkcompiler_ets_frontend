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

async function foo1(p: Promise<Promise<string>>): Promise<void> {
    let result: string = await p;
}

async function foo2(p: Promise<Promise<string>|number>): Promise<void> {
    let result: string|number = await p;
}

async function foo3(p: Promise<Promise<string>|Promise<number>>): Promise<void> {
    let result: string|number = await p;
}

async function foo4(p: Promise<null|Promise<string>>): Promise<void> {
    let result: null|string = await p;
}

type Alias = Promise<string>|number;
async function foo5(p: Promise<Alias>): Promise<void> {
    let result: string|number = await p;
}
