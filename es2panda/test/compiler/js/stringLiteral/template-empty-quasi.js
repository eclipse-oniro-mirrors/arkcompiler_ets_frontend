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

let trace = [];
function f() { trace.push("f"); return "F"; }
function g() { trace.push("g"); return "G"; }

let r1 = `${f()}${g()}`;
print(r1);
print(trace.join(","));

let primCount = 0;
let obj = { [Symbol.toPrimitive](hint) { primCount++; return "OBJ"; } };
let r2 = `${obj}`;
print(r2);
print(primCount);

let z = "Z";
let r3 = `o${`i${z}`}`;
print(r3);

let name = "world";
print(`Hello ${name}`);
print(`${name}`);

let a = "a", b = "b", c = "c";
print(`${a}${b}`);
print(`${a}${b}${c}`);
print(`${a}c`);
print(`plain`);

let x = "X", y = "Y";
print(`a${x}b${y}c`);

print(`${42}`);
print(`${1}${2}${3}`);

function tag(strings, ...values) { return strings[0] + values.join("") + strings[1]; }
print(tag`a${x}b`);
