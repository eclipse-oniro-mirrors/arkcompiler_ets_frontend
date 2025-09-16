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

a = 1;          // Right node with type number
a += 2;          // Right node with type number
a = (1 + "a");  // Right node with type TypeError
let b = "abc";
c = b;
let d = a + b + c;

function foo() {
    void = [([...asyncGenerator])]
}

/* @@? 21:9 Error TypeError: Unresolved reference a */
/* @@? 21:17 Error TypeError: Unresolved reference c */
/* @@? 24:5 Error SyntaxError: Unexpected token 'void'. */
/* @@? 24:18 Error TypeError: Unresolved reference asyncGenerator */
