/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

interface I 
{
   field? : String | Number;
}

abstract class C implements I
{
   field_? : Object | String;
   
   get field() : Object | String | undefined {
      return this.field_;
   }
   
   set field(o : Object | String | undefined) {
      this.field_ = o;
   }
}


/* @@? 25:13 Error Semantic error ESE985118: Class member field with type Object|String|undefined in C cannot implement field with type undefined|String|Double property from interface I because the implementing return type is not compatible with the interface return type. */
/* @@? 25:13 Error Semantic error ESE0136: Method field(): Object|String|undefined in C not overriding any method */