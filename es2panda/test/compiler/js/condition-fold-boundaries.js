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

// Each condition consumer must use only branch outcomes, never the scratch accumulator.
function check(ok, name) {
    if (!ok) { throw new Error(name); }
}
let values = [undefined, null, false, true, 0, -0, NaN, 1, "", "x", 0n, 1n, {}, [], Symbol("s")];
for (let a of values) {
    for (let b of values) {
        let trace = "";
        function left() { trace += "a"; return a; }
        function right() { trace += "b"; return b; }
        let actual = 0;
        if (!(left() && !right())) { actual = 1; }
        check(actual === Number(!(a && !b)), "nested not and");
        check(trace === (a ? "ab" : "a"), "and evaluation order");
        trace = "";
        if (!!(left() || !right())) { actual = 2; } else { actual = 3; }
        check(actual === ((a || !b) ? 2 : 3), "nested not or");
        check(trace === (a ? "a" : "ab"), "or evaluation order");
        check(Object.is(a && b, a ? b : a), "and value");
        check(Object.is(a || b, a ? a : b), "or value");
        check(Object.is(a ?? b, a === null || a === undefined ? b : a), "nullish value");
        check(typeof !a === "boolean" && !a === !Boolean(a), "not value");
    }
}
let n = 0;
while (!(n >= 3)) { n++; }
check(n === 3, "while");
do { n--; } while (!!(n > 0));
check(n === 0, "do while");
for (; !(n >= 3) && !!(n < 4); n++) {}
check(n === 3, "for");
check(!(1 < 2) ? false : true, "conditional");
let log = "";
let obj = { get p() { log += "get;"; return 1; } };
check(!!(obj.p < 2) && !("missing" in obj), "comparison and in");
check(log === "get;", "getter once");
class Private {
    #x;
    has(o) { return !(#x in o) ? false : true; }
}
let instance = new Private();
check(instance.has(instance) && !instance.has({}), "private in");
let thrown = false;
try { if (!("x" in null)) {} } catch (e) { thrown = e instanceof TypeError; }
check(thrown, "in exception");
print("condition fold boundaries passed");
