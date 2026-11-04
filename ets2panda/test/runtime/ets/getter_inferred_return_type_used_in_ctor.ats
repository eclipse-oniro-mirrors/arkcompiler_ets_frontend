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

// A getter without a return type annotation has its return type inferred from its body.
// The getter is read in a constructor declared before the getter, so the pending inference
// must be resolved on demand: the property access type must be the inferred class type,
// not the 'undefined' placeholder (previously miscompiled into a failing cast to
// 'undefined' and a ClassCastError at construction time).
class C {
    foo() {
        return this;
    }

    constructor(n: number) {
        let t = this;
        let v = t.y;
        arktest.assertTrue(isC(v));
    }

    get y() {
        return this;
    }
}

function isC(o: Object): boolean {
    return o instanceof C;
}

function main(): void {
    let c = new C(1);
    arktest.assertTrue(isC(c));
    arktest.assertTrue(isC(c.y));
    arktest.assertTrue(isC(c.foo()));
}
