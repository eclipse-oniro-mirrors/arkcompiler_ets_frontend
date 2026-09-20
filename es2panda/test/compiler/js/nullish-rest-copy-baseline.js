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

// Baseline record of the runtime's object rest copy behavior: which proxy
// traps fire while CopyDataProperties copies a Proxy source. Split from
// nullish-fold-boundaries.js because this observes runtime implementation
// behavior rather than the nullish fold emission itself.
let trace = "";
let proxy = new Proxy({p: 1}, {
    get(t, k) { trace += "get;"; return t[k]; },
    ownKeys(t) { trace += "keys;"; return Reflect.ownKeys(t); }
});
let {...rest} = proxy;
print("rest copy trace:" + trace);
