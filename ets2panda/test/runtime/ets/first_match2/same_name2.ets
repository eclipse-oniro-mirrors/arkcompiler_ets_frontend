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

class C {
    foo(a: number) {
        return "invoke1"
    }
}

class D extends C {
    foo(a: number) {
        return "invoke2"
    }
    foo(a: string) {
        return "invoke3"
    }
}

function main() {
    let d = new D();
    let c: C = d;
    arktest.assertEQ(d.foo(1), "invoke2");
    arktest.assertEQ(d.foo("abc"), "invoke3");
    arktest.assertEQ(c.foo(1), "invoke2");

    let c2 = new C();
    arktest.assertEQ(c2.foo(1), "invoke1");
}
