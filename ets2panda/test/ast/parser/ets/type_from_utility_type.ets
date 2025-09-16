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

 function foo<T>(){
   let partialA  = Type.from<Partial<T>>();
   let partialarrA  = Type.from<Partial<Array<string>>>();
   let partialarrT  = Type.from<Partial<Array<T>>>();

   let requiredA  = Type.from<Required<T>>();
   let requiredarrA  = Type.from<Required<Array<string>>>();
   let requiredarrT  = Type.from<Required<Array<T>>>();

   let readonlyA  = Type.from<Readonly<T>>();
   let readonlyarrA  = Type.from<Readonly<Array<string>>>();
   let readonlyarrT  = Type.from<Readonly<Array<T>>>();


   let recordA  = Type.from<Record<T,T>();
   let recordarrA  = Type.from<Record<Array<string>,Array<string>>>();
   let recordarrT  = Type.from<Record<Array<T>,Array<T>>>();
 }
 
  /* @@? 30:19 Error TypeError: Bad operand type, the types of the operands must be numeric, same enumeration, or boolean type. */
/* @@? 17:37 Error TypeError: T in Partial<T> must be a class or an interface type. */
/* @@? 21:39 Error TypeError: T in Required<T> must be a class or an interface type. */
/* @@? 30:19 Error TypeError: Bad operand type, the types of the operands must be numeric, same enumeration, or boolean type. */
/* @@? 30:29 Error TypeError: No static $_invoke method and static $_instantiate method in Record. Record() is not allowed. */
/* @@? 30:29 Error TypeError: Type 'Record<K,V>' has no call signatures. */
/* @@? 31:38 Error TypeError: Type argument 'Array<String>' should be a subtype of 'Numeric|String|BaseEnum<Int>|BaseEnum<Long>|BaseEnum<String>'-constraint */
/* @@? 32:38 Error TypeError: Type argument 'Array<T>' should be a subtype of 'Numeric|String|BaseEnum<Int>|BaseEnum<Long>|BaseEnum<String>'-constraint */
