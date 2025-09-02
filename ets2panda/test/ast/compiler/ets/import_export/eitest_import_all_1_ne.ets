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

import * as ns from "./eitest_import_all_2_ne"
const f: ns.qw.Root.Foo = {x:10}

/* @@? eitest_import_all_1_ne.ets:17:13 Error TypeError: 'qw' type does not exist. */
/* @@? eitest_import_all_1_ne.ets:17:16 Error TypeError: 'Root' type does not exist. */
/* @@? eitest_import_all_1_ne.ets:17:21 Error TypeError: 'Foo' type does not exist. */