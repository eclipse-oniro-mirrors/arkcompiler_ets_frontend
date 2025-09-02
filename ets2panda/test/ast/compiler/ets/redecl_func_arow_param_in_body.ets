/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the ermissions and
 * limitatioLicense.
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
 
function foo(bar: () => int) {
    let x , bar();
 }
 
 function main(): void {
   foo(() => {});
 }

/* @@? 18:11 Error SyntaxError: Variable must be initialized or it's type must be declared. */
/* @@? 18:13 Error TypeError: Variable 'bar' has already been declared. */
/* @@? 18:16 Error SyntaxError: Variable must be initialized or it's type must be declared. */
/* @@? 18:16 Error SyntaxError: Unexpected token '('. */
/* @@? 18:17 Error SyntaxError: Unexpected token ')'. */
/* @@? 18:18 Error SyntaxError: Unexpected token, expected ')'. */
/* @@? 22:14 Error TypeError: Type 'void' is not compatible with the enclosing method's return type 'Int' */
