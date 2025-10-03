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

import { f1, f2, a1, a2, a3, a4 } from "./export_type_several_times_2.ets"

f1()
f2()
let c1: a1 = new a1()
let c2: a2 = new a2()
c1 = a3
c2 = a4/* @@? export_type_several_times_2.ets:22:3 Warning Warning W32596: 'foo' is explicitly used with alias several times */
/* @@? export_type_several_times_2.ets:24:3 Warning Warning W32596: 'A' is explicitly used with alias several times */
/* @@? export_type_several_times_2.ets:26:3 Warning Warning W32596: 'a' is explicitly used with alias several times */