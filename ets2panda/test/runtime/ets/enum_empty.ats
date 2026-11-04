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

enum Empty { }

class TestClass { }

function main() {
    let a: Empty | undefined = undefined;
    let c: Empty | string | TestClass = "123"
    foo(a);
    foo2(c)
}

function foo(a?: Empty) { }

function foo2(a?: Empty | string | TestClass) {
    arktest.assertEQ(a, "123")
}
