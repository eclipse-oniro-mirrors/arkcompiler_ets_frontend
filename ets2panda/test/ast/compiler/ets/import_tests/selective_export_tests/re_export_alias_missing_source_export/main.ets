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

import { A } from "./router.ets"
import { B } from "./router.ets"

let a: A = new A()
let b: B = new B()

/* @@? main.ets:17:10 Error Semantic error ESE0356: Cannot find imported element 'B' */
/* @@? main.ets:20:8 Error Semantic error ESE0371: Cannot find type 'B'. */
/* @@? main.ets:20:16 Error Semantic error ESE0371: Cannot find type 'B'. */
/* @@? router.ets:22:10 Error Semantic error ESE0361: Incorrect export 'A' */
