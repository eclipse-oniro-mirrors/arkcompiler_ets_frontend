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

abstract struct X {
    x: number = 89
}

abstract struct B extends X {
    f(): void {
        this.x = 55
    }
}

function f(x: int): void {
    for (let i = 0; i < x; ++i) {
        aa:
            while (i < 10) {
                aa:
                    while (i > 10) {
                        ++i
                        break aa
                    }
                ++i
            }
    }
}

return 48;

function g(x: int): int {
    switch x
        case 1: return 0;
        case 2: return 1;
            77: return 2;
        default: return -1;
        default: return -2;
}
}



/* @@? 16:10 Error Semantic error ESE0364: Structs are only used to define UI components, it should be translated at 'plugin after parser' phase. */
/* @@? 20:10 Error Semantic error ESE0364: Structs are only used to define UI components, it should be translated at 'plugin after parser' phase. */
/* @@? 20:27 Error Semantic error ESE0177: struct B is not extensible. */
/* @@? 30:17 Error Syntax error ESY0214: Label already declared. */
/* @@? 40:1 Error Syntax error ESY0163: return keyword should be used in function body. */
/* @@? 40:8 Error Semantic error ESE0092: All return statements in the function should be empty or have a value. */
/* @@? 43:12 Error Syntax error ESY0230: Expected '(', got 'identification literal'. */
/* @@? 44:9 Error Syntax error ESY0230: Expected ')', got 'case'. */
/* @@? 44:9 Error Syntax error ESY0230: Expected '{', got 'case'. */
/* @@? 46:15 Error Syntax error ESY0227: Unexpected token ':'. */
/* @@? 46:17 Error Syntax error ESY0227: Unexpected token 'return'. */
/* @@? 48:9 Error Syntax error ESY0171: Multiple default clauses. */