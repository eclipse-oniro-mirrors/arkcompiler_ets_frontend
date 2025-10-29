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
  field1:number = 10;
  foo(){}
  private foo2(){}
}

function getProperty<K extends ((keyof A)|number|A)>(key: K){}

function main():void{
  let a:A = new A();

  /* @@ label1 */type keyofA = keyof A;
  /* @@ label2 */type keyofUnion = keyofA|number|A;
  let x1:/* @@ label3 */keyofUnion = "foo2";
  let x2:/* @@ label4 */keyofUnion = "other field";

  getProperty</* @@ label5 */keyofA>("foo2")
  getProperty</* @@ label6 */keyofA>("other field")
}


/* @@@ label1 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
/* @@@ label2 Error Syntax error ESY0040: Illegal start of Type Alias expression. */
/* @@@ label3 Error Semantic error ESE0371: Cannot find type 'keyofUnion'. */
/* @@@ label4 Error Semantic error ESE0371: Cannot find type 'keyofUnion'. */
/* @@@ label5 Error Semantic error ESE0371: Cannot find type 'keyofA'. */
/* @@@ label6 Error Semantic error ESE0371: Cannot find type 'keyofA'. */