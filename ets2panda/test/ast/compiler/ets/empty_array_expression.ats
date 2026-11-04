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

let var1 = /* @@ label1 */[];
const con1 = /* @@ label2 */[];

function foo(){
   let var2 = /* @@ label3 */[];
   const con2 = /* @@ label4 */[];
}

class A{
   prop1 = /* @@ label5 */[];
   static prop1 = /* @@ label6 */[];
   foo(){
      let var3 = /* @@ label7 */[];
      const con3 = /* @@ label8 */[];
   }
}


/* @@@ label1 Error Semantic error ESE0301: Can't resolve array type */
/* @@@ label2 Error Semantic error ESE0301: Can't resolve array type */
/* @@@ label3 Error Semantic error ESE0301: Can't resolve array type */
/* @@@ label4 Error Semantic error ESE0301: Can't resolve array type */
/* @@@ label5 Error Semantic error ESE0301: Can't resolve array type */
/* @@@ label6 Error Semantic error ESE0301: Can't resolve array type */
/* @@@ label7 Error Semantic error ESE0301: Can't resolve array type */
/* @@@ label8 Error Semantic error ESE0301: Can't resolve array type */