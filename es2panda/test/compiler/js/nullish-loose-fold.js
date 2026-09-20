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

/**
 * @file nullish checks are folded into a single loose `!= null` comparison.
 * Locks the runtime semantics of `?.`, `??`, `??=` and object-rest destructuring.
 */

// ?. : nullish short-circuits, falsy-but-defined continues
function opt(v) {
    return v?.x;
}
print(opt(null));
print(opt(undefined));
print(opt(0));
print(opt(false));
print(opt(""));
print(opt({x: 42}));

// ?.() : optional call form
function optCall(f) {
    return f?.(7);
}
print(optCall(null));
print(optCall(undefined));
print(optCall((n) => n + 1));

// ?? : only null/undefined take the right operand
print(null ?? "r");
print(undefined ?? "r");
print(0 ?? "r");
print(false ?? "r");
print("" ?? "r");
print("l" ?? "r");
print({}.x ?? "r");

// ??= : right side evaluated at most once, left reference evaluated exactly once
let a = null;
a ??= 5;
print(a);
let b = 0;
b ??= 6;
print(b);
let calls = 0;
function counter() {
    calls++;
    return null;
}
let obj = {};
obj[counter()] ??= (calls += 10, "v");
print(obj.p);
print(calls);

// object rest destructuring: null/undefined throw, everything else proceeds
function rest(v) {
    const {...others} = v;
    return JSON.stringify(others);
}
print(rest({p: 1, q: 2}));
print(rest(0));
try {
    rest(null);
} catch (e) {
    print(e.name);
}
try {
    rest(undefined);
} catch (e) {
    print(e.name);
}
