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

class A {
    field = ""
}

let a: A = {
    field: "1",
    "field": '2',
}

let b: A = {
    field: "1",
    field: "2",
    "field": '3',
}

let c: A = {
    "field": '2',
    "field": '3',
}


/* @@? 22:5 Error TypeError: An object literal cannot have multiple properties with the same name.  */
/* @@? 27:5 Error TypeError: An object literal cannot have multiple properties with the same name.  */
/* @@? 28:5 Error TypeError: An object literal cannot have multiple properties with the same name.  */
/* @@? 33:5 Error TypeError: An object literal cannot have multiple properties with the same name.  */
