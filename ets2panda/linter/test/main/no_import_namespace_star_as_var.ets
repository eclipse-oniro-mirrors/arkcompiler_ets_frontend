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

import * as host from './file1';
const alias = host; // error
alias.run();

function getHost() {
  return host; // error
}
const edgeCase = getHost();

function run(h: any) {
  h.doSomething();
}
run(host); // error

host(); // error

const { foo } = host; // error

let g;
g = host; // error

host.doSomething() // valid
new host.Host(); // valid

const h2 = host.h2(); // valid

function f1() {
  return host.b(); // valid
}

const fn = () => host.work(); // valid