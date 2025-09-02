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

class A{
  field1:int;
  field2:int;
  method1(){};
}

function getProperty(obj: A, key: keyof A){}

function main():void{
  let a = new A();
  /* @@ label1 */getProperty(a, /* @@ label2 */"other field");
  /* @@ label3 */getProperty(a, /* @@ label4 */"other method");
  /* @@ label5 */getProperty(a, /* @@ label6 */1);
}

/* @@@ label1 Error TypeError: No matching call signature for getProperty(A, "other field") */
/* @@@ label2 Error TypeError: Type '"other field"' is not compatible with type '"method1"|"field1"|"field2"' at index 2 */
/* @@@ label3 Error TypeError: No matching call signature for getProperty(A, "other method") */
/* @@@ label4 Error TypeError: Type '"other method"' is not compatible with type '"method1"|"field1"|"field2"' at index 2 */
/* @@@ label5 Error TypeError: No matching call signature for getProperty(A, Int) */
/* @@@ label6 Error TypeError: Type 'Int' is not compatible with type '"method1"|"field1"|"field2"' at index 2 */
