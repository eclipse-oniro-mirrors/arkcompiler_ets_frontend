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

import * as named from './namespace_export1.ets'
import * as production from './namespace_export2.ets'

arktest.assertEQ( /* @@ label1 */production, null, 'default specified via identifier');
arktest.assertEQ( /* @@ label2 */named, null, 'default specified via identifier');

/* @@? namespace_export1.ets:16:5 Warning Warning W71336: Exported variable 'x' requires type annotation. */

/* @@? namespace_export2.ets:1:3 Warning Warning W71336: Exported variable 'gensym%%_default_anonymous' requires type annotation. */

/* @@? namespace_import3.ets:19:1 Error Semantic error ESE0127: No matching call signature for assertEQ(production, null, "default specified via identifier") */
/* @@@ label1 Error Semantic error ESE43831: Namespace 'production' cannot be used as object */
/* @@? namespace_import3.ets:20:1 Error Semantic error ESE0127: No matching call signature for assertEQ(named, null, "default specified via identifier") */
/* @@@ label2 Error Semantic error ESE43831: Namespace 'named' cannot be used as object */
