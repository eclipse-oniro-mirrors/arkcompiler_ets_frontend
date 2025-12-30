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

class A {
    i?: number = 0
    foo() {}
}
class B extends A {
    j?: string = "str"
    bar() {}
}

function main() {
    let a: Required<B> = { i: 1.1, j: "str2" }
    /* @@ label */a.foo()
}

/* @@@ label Error Semantic error ESE123454: Cannot call methods of type 'Required'. */