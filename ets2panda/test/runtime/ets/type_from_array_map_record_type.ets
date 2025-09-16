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
  
function main(){
     let arrayA  = Type.from<Array<string>>();
     let arrayB  = Type.from<Array<string>>();
     let arrayC  = Type.from<string[]>();
     arktest.assertTrue(arrayA.toString() == arrayB.toString())
     arktest.assertTrue(arrayA.toString() == arrayC.toString())

     let mapA  = Type.from<Map<string,string>>();
     let mapB  = Type.from<Map<string,string>>();
     arktest.assertTrue(mapA.toString() == mapB.toString())

     let recordA  = Type.from<Record<string,string>>();
     let recordB  = Type.from<Record<string,string>>();
     arktest.assertTrue(recordA.toString() == recordB.toString())

}