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

import { A } from "./same_named_origin_b.ets"
import { callback } from "./same_named_origin_callback.ets"

class C {
    mycallback: (myA: A) => void = /* @@ label */callback
}

/* @@@ label Error Semantic error ESE0318: Type '((p1: A) => undefined)' cannot be assigned to type '((p1: A) => undefined)' */
/* @@@ label Error Semantic error ESE249556: Type 'A' comes from both 'same_named_origin_a' and 'same_named_origin_b'. */
