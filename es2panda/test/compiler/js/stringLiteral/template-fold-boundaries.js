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

function check(ok, name) {
    if (!ok) { throw new Error(name); }
}
check(`${1}${2}` === "12" && typeof `${1}` === "string", "first quasi");
check(`` === "" && `plain` === "plain", "no substitutions");
check(`${0n}${false}${null}${undefined}` === "0falsenullundefined", "coercion");
check(`${1}\
${2}` === "12", "empty cooked nonempty raw");
let trace = "";
let value = { [Symbol.toPrimitive]() { trace += "convert;"; return "x"; } };
function next() { trace += "next;"; return 2; }
check(`${value}${next()}` === "x2" && trace === "convert;next;", "conversion order");
trace = "";
let bad = { [Symbol.toPrimitive]() { trace += "throw;"; throw new Error("stop"); } };
try { `${bad}${next()}`; } catch (e) { check(e.message === "stop", "exception"); }
check(trace === "throw;", "exception stops next expression");
function tag(strings, ...values) {
    check(strings.length === 3 && strings.every(s => s === ""), "tagged empty quasis");
    check(strings.raw.length === 3 && strings.raw.every(s => s === ""), "tagged raw");
    check(values[0] === value && values[1] === bad, "tagged no conversion");
    return strings;
}
function site() { return tag`${value}${bad}`; }
// Compare template-object caching with the baseline on the same runtime.
print("template identity:" + (site() === site()));
function invalid(strings) {
    check(strings[0] === undefined && strings.raw[0] === "\\unicode", "undefined cooked");
}
invalid`\unicode`;
check(String.raw`${1}${2}` === "12", "String.raw");
print("template fold boundaries passed");
