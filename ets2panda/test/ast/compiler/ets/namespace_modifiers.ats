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

namespace V {
    class A {
        readonly const P = 0
    }
}

class B {
    readonly const Q = 0
}

namespace W {
    readonly R = 0
}

namespace X {
    namespace W {
        const S = 0
    }
}

namespace M {
    namespace N {
        namespace O {
            const T = 0
        }
    }
}


/* @@? 18:18 Error Syntax error ESY0227: Unexpected token 'const'. */
/* @@? 23:14 Error Syntax error ESY0227: Unexpected token 'const'. */
/* @@? 27:5 Error Semantic error ESE0143: Unresolved reference readonly */
/* @@? 27:14 Error Syntax error ESY0227: Unexpected token 'R'. */
/* @@? 27:14 Error Semantic error ESE0143: Unresolved reference R */
/* @@? 27:16 Error Syntax error ESY0141: 'readonly' type modifier is only permitted on resizable array and tuple types. */