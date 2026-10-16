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

// 4a: exported function references a local type in its return type
class Holder {
    value: string = '';
}
export function makeHolder(): Holder {
    return new Holder();
}
// 4b: exported variable references a local type in its annotation
class Config {
    name: string = '';
}
export const defaultConfig: Config = new Config();
