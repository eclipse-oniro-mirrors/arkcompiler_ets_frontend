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

let arg='2';
let v:null|int=null;
class A{

}
let a:A = new A();
switch (arg){
    case '0':
    case '1':
    case '2':
        break;
    case '2':
        break;
    case '2':
        break;
    case 2:
        break;
    case v:
        break;
    case A:
        break;
}

/* @@? 31:10 Error TypeError: Switch case type 'int' is not comparable to discriminant type 'String'  */
/* @@? 33:10 Error TypeError: Switch case type 'null|Int' is not comparable to discriminant type 'String'  */
/* @@? 35:10 Error TypeError: Class name 'A' used in the wrong context */
/* @@? 35:10 Error TypeError: Switch case type 'A' is not comparable to discriminant type 'String'  */
/* @@? 33:5 Error TypeError: Constant expression required */
/* @@? 35:5 Error TypeError: Constant expression required */
/* @@? 27:5 Error TypeError: Case duplicate */
/* @@? 29:5 Error TypeError: Case duplicate */
/* @@? 31:5 Error TypeError: Case duplicate */
