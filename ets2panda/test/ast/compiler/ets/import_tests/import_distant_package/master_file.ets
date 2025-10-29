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

import { moreImportantInfo, notInit, notExist } from "./package_with_errors"

console.log(notExist)


/* @@? import_in_package_with_error.ets:20:17 Error Semantic error ESE0371: Cannot find type 'notInit'. */
/* @@? import_in_package_with_error.ets:20:29 Error Syntax error ESY0227: Unexpected token '='. */
/* @@? import_in_package_with_error.ets:20:30 Error Syntax error ESY0228: Unexpected token, expected ')'. */
/* @@? import_in_package_with_error.ets:20:32 Error Syntax error ESY0227: Unexpected token ')'. */

/* @@? distant_package.ets:22:22 Error Syntax error ESY0105: Variable must be initialized or it's type must be declared. */

/* @@? package_with_errors_1.ets:19:32 Error Semantic error ESE0326: Cannot cast type 'String' to 'Double' */

/* @@? package_with_errors_2.ets:19:16 Error Semantic error ESE0143: Unresolved reference foo */

/* @@? master_file.ets:16:54 Error Semantic error ESE0356: Cannot find imported element 'notExist' */
/* @@? master_file.ets:18:1 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Double): void` */
/* @@? master_file.ets:18:1 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Float): void` */
/* @@? master_file.ets:18:1 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Long): void` */
/* @@? master_file.ets:18:1 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Int): void` */
/* @@? master_file.ets:18:1 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Char): void` */
/* @@? master_file.ets:18:1 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Short): void` */
/* @@? master_file.ets:18:1 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Byte): void` */
/* @@? master_file.ets:18:1 Error Semantic error ESE0142: Call to `log` is ambiguous as `2` versions of `log` are available: `log(i: String): void` and `log(i: Boolean): void` */
/* @@? master_file.ets:18:13 Error Semantic error ESE0143: Unresolved reference notExist */