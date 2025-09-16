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
   interface C{
   }

   interface D{
   }
  function main(){
     let classA  = Type.from<A>();
     let classB  = Type.from<B>();
     let classC  = Type.from<A>();
     arktest.assertTrue(classA.toString() == classC.toString())
     arktest.assertTrue(classA.toString() != classB.toString())

     let interfaceA  = Type.from<C>();
     let interfaceB  = Type.from<D>();
     let interfaceC  = Type.from<C>();
     arktest.assertTrue(interfaceA.toString() == interfaceC.toString())
     arktest.assertTrue(interfaceA.toString() != interfaceB.toString())
}