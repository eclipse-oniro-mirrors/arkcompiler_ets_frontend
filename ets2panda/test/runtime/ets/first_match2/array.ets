/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

class Test<T> {
    foo(a: FixedArray<T>): string {
        return "invoke1";
    }
    foo(a: FixedArray<FixedArray<T>>): string {
        return "invoke5";
    }
}

function main() {
    let s: Test<string> = new Test<string>();
    arktest.assertEQ(s.foo(["abc", "abc", "abc"]), "invoke1");
    arktest.assertEQ(s.foo(new FixedArray<string>(5)), "invoke1");
}
