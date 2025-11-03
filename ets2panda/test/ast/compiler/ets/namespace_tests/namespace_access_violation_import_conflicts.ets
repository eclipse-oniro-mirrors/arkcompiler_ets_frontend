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
// Example of accessing non-exported members in merged namespaces

import { Space2 as Space1 } from "./src02";

if (Space1.variable == Space1.constant) { // CTE
    Space1.variable = 4321; // CTE
}

Space1.foo(); // CTE
Space1.foo(1234); //ok

/* @@? 19:31 Error TypeError: Property 'constant' does not exist on type 'Space2' */
/* @@? 20:23 Error TypeError: Type 'Int' cannot be assigned to type 'String' */
/* @@? 23:1 Error TypeError: Expected 1 arguments, got 0. */
/* @@? 23:1 Error TypeError: No matching call signature for foo() */