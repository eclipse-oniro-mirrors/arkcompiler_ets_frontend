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

class Person {
  name: string = ""
}
let p = new Person()
let b = "name" /* @@ label1 */in p
let c = "name" /* @@ label2 */in {name: 1, some: 2}
let d = 2 /* @@ label3 */in [44, 55]


/* @@@ label1 Error Syntax error ESY105822: 'in' operator is not supported. Use 'instanceof' operator to check whether an object is the instance of a class that contains the necessary class member. */
/* @@@ label2 Error Syntax error ESY105822: 'in' operator is not supported. Use 'instanceof' operator to check whether an object is the instance of a class that contains the necessary class member. */
/* @@@ label3 Error Syntax error ESY105822: 'in' operator is not supported. Use 'instanceof' operator to check whether an object is the instance of a class that contains the necessary class member. */