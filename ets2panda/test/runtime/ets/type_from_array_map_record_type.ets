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
  
function main(){
     let arrayA  = Class.from<Array<string>>();
     let arrayB  = Class.from<Array<string>>();
     let arrayC  = Class.from<string[]>();
     arktest.assertTrue(arrayA == arrayB)
     arktest.assertTrue(arrayA == arrayC)

     let mapA  = Class.from<Map<string,string>>();
     let mapB  = Class.from<Map<string,string>>();
     arktest.assertTrue(mapA == mapB)

     let recordA  = Class.from<Record<string,string>>();
     let recordB  = Class.from<Record<string,string>>();
     arktest.assertTrue(recordA == recordB)
}
