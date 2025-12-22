/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

package mypack;

let wont_exist: number = 8;

// this file will contain parse error, but compilation of 'package_module_1' is not aborted, file is just ignored

syntax error


/* @@? 22:1 Error Syntax error ESY0291: Invalid package toplevel statement */
/* @@? 22:1 Error Semantic error ESE0143: Unresolved reference syntax */
/* @@? 22:8 Error Syntax error ESY0227: Unexpected token 'error'. */
/* @@? 22:8 Error Syntax error ESY0291: Invalid package toplevel statement */
/* @@? 22:8 Error Semantic error ESE0143: Unresolved reference error */