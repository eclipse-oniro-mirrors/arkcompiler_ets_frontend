/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

interface A {
    callback: Object;
}

class B {
    foo(): void { };
}

class C implements A {
    private b?: B;
    constructor(b: B) {
        this.b = b;
    }
    public callback: Object = (c: String): String => {
        this.b?.foo();
        return c;
    }
}

function main() {
    let b = new B();
    let temp = new C(b);
    const s = (temp.callback as (c: String) => String)("hello");
    arktest.assertEQ(s, "hello");
}
