/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

enum A {
    B, C, D
}

enum E {
    F, G, H
}
class Fgr {
    readonly static BR = A.B
}

class Sqr {
    readonly static BR = E.F
}

function main() {
    let a = E.H
    switch (a) {
        case /* @@ label */Fgr.BR:
         break;
        case Sqr.BR:
        break;
        default: break;
    }
}


/* @@@ label Error Semantic error ESE0262: Switch case type 'A' is not comparable to discriminant type 'E' */
/* @@@ label Error Semantic error ESE0334: Enum switch case must be unqualified name of an enum constant */
/* @@? 36:14 Error Semantic error ESE0334: Enum switch case must be unqualified name of an enum constant */