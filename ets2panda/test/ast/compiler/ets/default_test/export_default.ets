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

const val = 100;
export { val as default };

export { default } from '.'; 

/* @@? 19:8 Error TypeError: Only one default export is allowed in a module  */
/* @@? 19:18 Error SyntaxError: Unexpected token, expected 'as'.  */
/* @@? 19:18 Error SyntaxError: Unexpected token '}'.  */
/* @@? 19:20 Error SyntaxError: Cannot find name 'from' to export.  */
/* @@? 19:20 Error SyntaxError: Cannot find name 'gensym%%_anonymous_const' to export.  */
/* @@? 19:25 Error SyntaxError: Unexpected token, expected ',' or '}'.  */
