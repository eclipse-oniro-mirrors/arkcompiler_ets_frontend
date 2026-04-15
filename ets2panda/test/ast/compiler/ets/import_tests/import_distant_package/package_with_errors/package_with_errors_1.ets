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

// "importantInfo" is imported in "import_in_package_with_error.ets"
export let moreImportantInfo = /* @@ label */importantInfo as number;

/* @@? 16:9 Error Syntax error ESY139148: The package keyword is deprecated. */
/* @@? 19:12 Warning Warning W71336: Exported variable 'moreImportantInfo' requires type annotation. */
/* @@@ label Error Semantic error ESE0143: Unresolved reference importantInfo */
