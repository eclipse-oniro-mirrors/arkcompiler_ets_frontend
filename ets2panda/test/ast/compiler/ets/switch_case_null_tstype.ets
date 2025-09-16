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

let arg='2';
let v null|int=null;
class A{}
switch (arg){
    case v:
        break;
    case A:
        break;
}

/* @@? 17:7 Error SyntaxError: Variable must be initialized or it's type must be declared. */
/* @@? 17:7 Error SyntaxError: Unexpected token 'null'. */
/* @@? 17:7 Error TypeError: Bad operand type, the types of the operands must be numeric type. */
/* @@? 17:12 Error SyntaxError: Unexpected token 'int'. */
/* @@? 22:5 Error TypeError: Constant expression required */
/* @@? 22:10 Error SyntaxError: Class cannot be used as object. */
/* @@? 22:10 Error TypeError: Switch case type 'A' is not comparable to discriminant type 'String' */
