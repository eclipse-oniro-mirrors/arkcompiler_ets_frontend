/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http: //www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

interface I<T> { }

class B <T>{ }

class A <T> extends B <T> implements I <T> { }

function foo(i: Partial<I<String>>, b: Partial<B<String>>): number{
    return 1.0
}
function bar(b: Partial<A<String>>): number{
    return foo(b, b)
}

function main() {
    let input: Partial<A<String>> = {}
    arktest.assertEQ(bar(input), 1.0)
}
