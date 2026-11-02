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

// Bytecode feature gated at target API 26.1.0: by-name accesses on the
// current frame `this` must be folded into ldthisbyname / stthisbyname, and
// `this` materializations whose `this` binding is lexically captured (the
// arrows make bump/current capture it) into ldthis, for 26.1.0 and later
// targets. Every lower target keeps the generic ldlexvar + ldobjbyname /
// stobjbyname forms. Both emissions are value-identical, so the printed
// output must not depend on the compile target.
class Counter {
    constructor() {
        this.value = 0;
    }
    bump(step) {
        const keep = () => this.value;
        this.value += step;
        keep();
        return this;
    }
    current() {
        const probe = () => this.value;
        probe();
        return this.value;
    }
}

const counter = new Counter();
counter.bump(2).bump(3);
print(counter.current());
