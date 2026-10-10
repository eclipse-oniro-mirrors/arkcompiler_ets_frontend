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
 * @file nullish checks are folded into a single loose `!= null` (noteq) comparison:
 * `?.` / object-rest destructuring jump to the nullish block on jeqz,
 * `??` / `??=` jump to the keep-left block on jnez. No strictnoteq pairs remain.
 */

// ?. — noteq + jeqz to the short-circuit block
function opt(v) {
    return v?.x;
}
opt(null);
opt({x: 1});

// ?.() — optional call keeps the this-vreg handling of the short-circuit block
function optCall(f) {
    return f?.(1);
}
optCall(null);
optCall((n) => n);

// ?? — noteq + jnez to the keep-left block
function coal(v) {
    return v ?? 0;
}
coal(null);
coal(0);

// ??= — same polarity as ??
function coalAssign(o) {
    o.p ??= 1;
    return o.p;
}
coalAssign({});
coalAssign({p: 2});

// object rest destructuring — noteq + jeqz to the throw block
function rest(v) {
    const {...others} = v;
    return others;
}
rest({a: 1});

// Variable assignment uses the same single nullish comparison as property assignment.
function coalAssignVariable(a, b) {
    a ??= b;
    return a;
}
coalAssignVariable(null, 1);
coalAssignVariable(0, 1);

// Assignment destructuring must also use a single nullish comparison.
function restAssign(obj) {
    let r;
    ({...r} = obj);
    return r;
}
restAssign({a: 1});
