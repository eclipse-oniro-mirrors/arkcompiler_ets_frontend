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

package package_with_errors

// "importantInfo" is imported in "import_in_package_with_error.ets"
export let moreImportantInfo = importantInfo as number;

/* @@? import_in_package_with_error.ets:20:17 Error TypeError: Cannot find type 'notInit'. */
/* @@? import_in_package_with_error.ets:20:29 Error SyntaxError: Unexpected token '='. */
/* @@? import_in_package_with_error.ets:20:30 Error SyntaxError: Unexpected token, expected ')'. */
/* @@? import_in_package_with_error.ets:20:32 Error SyntaxError: Unexpected token ')'. */
/* @@? distant_package.ets:22:22 Error SyntaxError: Variable must be initialized or it's type must be declared. */
/* @@? package_with_errors_2.ets:19:16 Error TypeError: Unresolved reference foo */
/* @@? package_with_errors_2.ets:19:16 Error TypeError: This expression is not callable. */
/* @@? package_with_errors_1.ets:19:32 Error TypeError: Cannot cast type 'String' to 'Double' */
