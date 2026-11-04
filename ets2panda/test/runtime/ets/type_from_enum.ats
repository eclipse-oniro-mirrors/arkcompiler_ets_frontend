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

enum Color { Green = "red" , Red = "blue", Blue = "green" }
enum ColorNumber { Green = 1 , Red = 2, Blue = 3 }



 function main(){
   let enumColorA = Class.from<Color>();
   let enumColorB = Class.from<Color>();
   let enumColorC = Class.from<ColorNumber>();
   arktest.assertTrue(enumColorA == enumColorB);
   arktest.assertTrue(enumColorA != enumColorC);
 }
