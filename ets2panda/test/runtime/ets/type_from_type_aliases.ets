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
  * See the License from the specific language governing permissions and
  * limitations under the License.
  */
class A{
}

interface B{
}

namespace MyNamespace {
    export interface Person {
        name: string;
        age: number;
    }

    export type EmployeeA = Person;
    export type EmployeeB = Person;
    export type stringType = string;
}

type intType =  int;
type stringType =  string;
type functionType = ()=>void;
type aClass = A;
type bInterface = B;
type arrayType =  Array<string>;
type mapType =  Map<string,string>;
type recordType =  Record<string,string>;

function main(){
   let a = Type.from<intType>();
   let b = Type.from<int>();
   arktest.assertTrue(a == b);

   let stringA = Type.from<stringType>();
   let stringB = Type.from<string>();
   arktest.assertTrue(stringA.toString() == stringB.toString());

   let c = Type.from<aClass>()
   let d = Type.from<A>()
   arktest.assertTrue(c.toString() == d.toString());

   let e = Type.from<bInterface>()
   let f = Type.from<B>()
   arktest.assertTrue(e.toString() == f.toString());

   let g = Type.from<arrayType>()
   let h = Type.from<Array<string>>()
   arktest.assertTrue(g.toString() == h.toString());


   let mapA = Type.from<mapType>();
   let mapB = Type.from<Map<string,string>>();
   arktest.assertTrue(mapA.toString() == mapB.toString());

   let recordA = Type.from<recordType>();
   let recordB = Type.from<Record<string,string>>();
   arktest.assertTrue(recordA.toString() == recordB.toString());


   let functionA = Type.from<functionType>();
   let functionB = Type.from<()=>void>();
   arktest.assertTrue(functionA.toString() == functionB.toString());

   let employeeA = Type.from<MyNamespace.EmployeeA>();
   let employeeB = Type.from<MyNamespace.EmployeeB>();
   let employeeC = Type.from<MyNamespace.stringType>();
   arktest.assertTrue(employeeA.toString() == employeeB.toString());
   arktest.assertTrue(employeeA.toString() != employeeC.toString());

}