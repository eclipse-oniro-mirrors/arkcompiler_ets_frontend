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

type Action = "verify";
type ContentMatch = "match";
type Outcome = `${Action}_${ContentMatch}`;
type Outcome2 = `${Action}_match`;
type Outcome3 = `verify_${ContentMatch}`;

type Foo = "Foo";
type Bar = "Bar";

let template: `${Foo}_${Bar}`;
function foo(template: `${Foo}_${Bar}`): `${number}` {}
