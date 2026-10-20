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

// A method without a return type annotation is used as a value before its declaration
// is checked: the pending return type inference must be resolved on demand, so the
// method reference gets the inferred arrow type (() => C), not (() => undefined).
class C {
    constructor(n: number) {
        let f: () => C = this.bar;
        arktest.assertTrue(isC(f()));
    }

    bar() {
        return this;
    }
}

function isC(o: Object): boolean {
    return o instanceof C;
}

function main(): void {
    let c = new C(1);
    arktest.assertTrue(isC(c));
}
