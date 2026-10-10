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

var name = "world";
var a = "a", b = "b", c = "c", x = "x", y = "y";

var t1 = `Hello ${name}`;
var t2 = `${name}`;
var t3 = `${a}${b}`;
var t4 = `${a}${b}${c}`;
var t5 = `${x}c`;
var t6 = `Hello`;
var t7 = `a${x}b${y}c`;

function tag(s) { return s[0]; }
var t8 = tag`x${y}`;
