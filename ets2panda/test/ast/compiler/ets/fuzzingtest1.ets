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

// the test case is from fuzzer.
/* @@ label */fuzzz./* @@ label2 */@@/* @@ label3 */@@

/* @@@ label Error TypeError: Unresolved reference fuzzz  */
/* @@@ label2 Error SyntaxError: There is no any node to insert at the placeholder position. */
/* @@@ label2 Error SyntaxError: Identifier expected, got '@@'. */
/* @@@ label3 Error SyntaxError: Unexpected token '@@'. */