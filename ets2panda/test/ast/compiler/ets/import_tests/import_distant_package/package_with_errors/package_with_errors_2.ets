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

package package_with_errors

// "notInit" is imported in "import_in_package_with_error.ets"
let newValue = foo(notInit)

export notInit

/* @@? package_with_errors_2.ets:19:16 Error Semantic error ESE0143: Unresolved reference foo */
/* @@? package_with_errors_2.ets:21:8 Error Syntax error ESY0279: Cannot find name 'notInit' to export. */
