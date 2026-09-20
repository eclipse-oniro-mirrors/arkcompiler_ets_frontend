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
let coercions = 0;
let object = { [Symbol.toPrimitive]() { coercions++; throw new Error("coercion"); } };
let revoked = Proxy.revocable({}, {});
revoked.revoke();
for (let value of [undefined, null, false, 0, -0, NaN, "", 0n, 1n, Symbol("s"), object, revoked.proxy]) {
    let nullish = value === null || value === undefined;
    let calls = 0;
    function fallback() { calls++; return "fallback"; }
    check(Object.is(value ?? fallback(), nullish ? "fallback" : value), "coalesce identity");
    check(calls === Number(nullish), "coalesce side effect");
    let holder = { p: value };
    let gets = 0, sets = 0, refs = 0;
    let proxy = new Proxy(holder, {
        get(t, k) { gets++; return t[k]; },
        set(t, k, v) { sets++; t[k] = v; return true; }
    });
    function reference() { refs++; return proxy; }
    calls = 0;
    let result = reference().p ??= fallback();
    check(Object.is(result, nullish ? "fallback" : value), "assignment value");
    check(gets === 1 && sets === Number(nullish) && refs === 1 && calls === Number(nullish), "assignment effects");
}
check(coercions === 0, "no ToPrimitive");
let keys = 0;
check(null?.[keys++] === undefined && keys === 0, "optional key skipped");
let obj = { p: 3, method() { return this.p; } };
check(obj.method?.() === 3, "optional this");
let trace = "";
let proxy = new Proxy({p: 1}, {
    get(t, k) { trace += "get;"; return t[k]; },
    ownKeys(t) { trace += "keys;"; return Reflect.ownKeys(t); }
});
check(proxy?.p === 1 && trace === "get;", "optional proxy");
let {...rest} = proxy;
check(rest.p === 1, "rest proxy value");
function defaults(a = 7) { return a; }
check(defaults(null) === null && defaults(undefined) === 7, "undefined-only default");
let thrown = false;
try { let {...rest} = null; } catch (e) { thrown = e instanceof TypeError; }
check(thrown, "rest null throws");
print("nullish fold boundaries passed");
