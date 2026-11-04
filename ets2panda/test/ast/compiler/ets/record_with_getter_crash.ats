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

interface I {
    meth(): Record<string, number>
}
interface J {
    meth(): Record<string, number>
}

class A implements I {
    meth(): Record<string, number> {
        return {
            get a(): number {
                return 11.0
            }
        }
    }
}

class B implements J {
    meth(): Record<string, number> {
        return { 'b': 22.0}
    }
}

let u1: I | J = new A()
let u2: I | J = new B()

function main() {
    assertEQ(u1.meth()['a'], 11.0)
    assertEQ(u2.meth()['b'], 22.0)
}


/* @@? 26:17 Error Syntax error ESY0062: Object pattern can't contain methods. */
/* @@? 43:5 Error Semantic error ESE0143: Unresolved reference assertEQ */
/* @@? 44:5 Error Semantic error ESE0143: Unresolved reference assertEQ */
