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

function main(): void {
    arktest.assertEQ(foo("1"), 3);
}

class A { }
class B { }

function foo2(x: string, y?: A) {
    return 1
}
function foo3(x: string, y?: B) {
    return 2
}
function foo1(x: string) {
    return 3
}

overload foo{ foo1, foo2, foo3 }
