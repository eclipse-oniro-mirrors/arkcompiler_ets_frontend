/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

import * as ns from './namespace_export3.ets'

arktest.assertEQ(/* @@ label1 */ns, 1)

/* @@? namespace_export3.ets:17:9 Warning Warning W71336: Exported class property 'x' requires type annotation. */

/* @@? namespace_import4.ets:18:1 Error Semantic error ESE0127: No matching call signature for assertEQ(ns, Int) */
/* @@@ label1 Error Semantic error ESE43831: Namespace 'ns' cannot be used as object */
