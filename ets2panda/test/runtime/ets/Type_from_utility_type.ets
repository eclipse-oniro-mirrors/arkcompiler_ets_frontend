 /*
  * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
   let partialA  = Class.from<Partial<A>>();
   let partialB  = Class.from<Partial<B>>();
   let partialC  = Class.from<Partial<A>>();
   arktest.assertTrue(partialA != partialB);
   arktest.assertTrue(partialA == partialC);

   let requiredA  = Class.from<Required<A>>();
   let requiredB  = Class.from<Required<B>>();
   let requiredC  = Class.from<Required<A>>();
   arktest.assertTrue(requiredA != requiredB);
   arktest.assertTrue(requiredA == requiredC);

  let readonlyA  = Class.from<Readonly<A>>();
  let readonlyB  = Class.from<Readonly<B>>();
  let readonlyC  = Class.from<Readonly<A>>();
  arktest.assertTrue(readonlyA != readonlyB);
  arktest.assertTrue(readonlyA == readonlyC);

  let recordA  = Class.from<Record<string,string>>();
  let recordB  = Class.from<Record<string,string>>();
  arktest.assertTrue(recordA == recordB);

  let partialarrayA  = Class.from<Partial<Array<string>>>();
  let partialarrayB  = Class.from<Partial<Array<int>>>();
  arktest.assertTrue(partialarrayA == partialarrayB);

 }
