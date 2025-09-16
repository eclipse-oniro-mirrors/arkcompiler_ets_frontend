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

 class B{
 }

 function main(){
   let partialA  = Type.from<Partial<A>>();
   let partialB  = Type.from<Partial<B>>();
   let partialC  = Type.from<Partial<A>>();
   arktest.assertTrue(partialA.toString() != partialB.toString());
   arktest.assertTrue(partialA.toString() == partialC.toString());

   let requiredA  = Type.from<Required<A>>();
   let requiredB  = Type.from<Required<B>>();
   let requiredC  = Type.from<Required<A>>();
   arktest.assertTrue(requiredA.toString() != requiredB.toString());
   arktest.assertTrue(requiredA.toString() == requiredC.toString());

  let readonlyA  = Type.from<Readonly<A>>();
  let readonlyB  = Type.from<Readonly<B>>();
  let readonlyC  = Type.from<Readonly<A>>();
  arktest.assertTrue(readonlyA.toString() != readonlyB.toString());
  arktest.assertTrue(readonlyA.toString() == readonlyC.toString());

  let recordA  = Type.from<Record<string,string>>();
  let recordB  = Type.from<Record<string,string>>();
  arktest.assertTrue(recordA.toString() == recordB.toString());

  let partialarrayA  = Type.from<Partial<Array<string>>>();
  let partialarrayB  = Type.from<Partial<Array<int>>>();
  arktest.assertTrue(partialarrayA.toString() == partialarrayB.toString());

 }