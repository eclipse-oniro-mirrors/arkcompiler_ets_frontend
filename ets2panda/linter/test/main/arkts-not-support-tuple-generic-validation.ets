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

let p1: Promise<string> = new Promise((resolve) => resolve("ok"));
let p2: Promise<number> = new Promise((resolve) => resolve(1));

Promise.all<[Promise<string>, Promise<number>]>([p1, p2]); // error
Promise.any<[Promise<string>, Promise<number>]>([p1, p2]); // error
Promise.race<[Promise<string>, Promise<number>]>([p1, p2]); // error
Promise.allSettled<[Promise<string>, Promise<number>]>([p1, p2]); // error

Promise.all<string | number>([p1, p2]); // valid
Promise.any<string | number>([p1, p2]); // valid
