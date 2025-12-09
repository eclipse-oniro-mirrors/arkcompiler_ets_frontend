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

import * as Test from "import_tests/packages";

// Throw error: entities should only be reachable via Test alias.

let x = foo(dbl /* double based signature */);


/* @@? 20:9 Error Semantic error ESE0143: Unresolved reference foo */
/* @@? 20:9 Error Semantic error ESE0289: This expression is not callable. */