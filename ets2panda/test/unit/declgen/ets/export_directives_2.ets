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

import {A1, foo78} from './expected/export_directives_1.txt'
import A2 from './expected/export_directives_1.txt'

export {A1} from './expected/export_directives_1.txt';

export {A2 as default};

export function foo(x: A2) { return x.foo77(); }

let x = new A2().foo77();

export function bar(x: A2): A1 { return foo78(new A1()); }
