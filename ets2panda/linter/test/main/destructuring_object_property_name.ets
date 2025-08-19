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

let obj = {
  a: 1,
  '2': 2,
  3: 3,
  ['d']: 4
};

let { a, a: a2 } = obj; // OK
let { '2': b } = obj; // Not fixable, support only identifiers as property names
let { 3: c } = obj; // Not fixable, support only identifiers as property names
let { ['d']: d } = obj; // Not fixable, support only identifiers as property names

({ a, a: a2 } = obj); // OK
({ '2': b } = obj); // Not fixable, support only identifiers as property names
({ 3: c } = obj); // Not fixable, support only identifiers as property names
({ ['d']: d } = obj); // Not fixable, support only identifiers as property names