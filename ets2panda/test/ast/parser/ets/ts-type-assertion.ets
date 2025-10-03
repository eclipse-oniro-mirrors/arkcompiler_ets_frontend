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

const a = 1;
type t = number;
const b = <string> a;
const c = <t> foo;
const d = (<t>foo);
const e = <t>(foo);
const f = <t><t>foo;

/* @@? 18:11 Error Syntax error ESY178094: Type cast syntax '<type>' is not supported, please use 'as' keyword instead */
/* @@? 19:11 Error Syntax error ESY178094: Type cast syntax '<type>' is not supported, please use 'as' keyword instead */
/* @@? 20:12 Error Syntax error ESY178094: Type cast syntax '<type>' is not supported, please use 'as' keyword instead */
/* @@? 21:11 Error Syntax error ESY178094: Type cast syntax '<type>' is not supported, please use 'as' keyword instead */
/* @@? 22:11 Error Syntax error ESY178094: Type cast syntax '<type>' is not supported, please use 'as' keyword instead */
/* @@? 22:14 Error Syntax error ESY178094: Type cast syntax '<type>' is not supported, please use 'as' keyword instead */