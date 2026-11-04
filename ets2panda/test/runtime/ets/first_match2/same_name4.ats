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

interface I {
    foo(a: number){
        return "invoke1"
    }
}

interface J extends I {
    foo(a: string): string
}

class C implements I {
    foo(a: number) {
        return "invoke2"
    }
    foo(a: string) {
        return "invoke3"
    }
}

function foo(param: J) {
    param.foo(1);
    param.foo("abc")
}

function main() {
    let c = new C();
    arktest.assertEQ(c.foo(1), "invoke2");
    arktest.assertEQ(c.foo("abc"), "invoke3");
}
