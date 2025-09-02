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
 

import {foo, m, n} from "./interop_import_js_compare_js"
let a = foo.getProperty("a")
let b = foo.getProperty("b")
a > b
a < b
a >= b
a <= b
a = 1

m.toNumber() > n.toNumber()
m = 1

let x = 1, y = 2;
x > y;
x < y;
x >= y;
x <= y;

interface GeneratedObjectLiteralInterface_1 {
    a: number;
    b: number;
}
let bar: GeneratedObjectLiteralInterface_1 = { a: 1, b: 2 };

let x2 = bar.a, y2 = bar.b;
x2 > y2;
x2 < y2;
x2 >= y2;
x2 <= y2;

foo.getProperty("a").toNumber() > foo.getProperty("b").toNumber();