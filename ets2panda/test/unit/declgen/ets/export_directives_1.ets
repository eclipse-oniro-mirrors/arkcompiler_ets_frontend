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

class A1 {
  constructor() {
    console.println("A1");
  }
}

export default A2;

type A3 = A1

class A2 {
   foo77() {
      console.println("foo77");
      return new A1();
   }
   y: A1 = new A1()
}

export {A1 as A1}

export function foo78(x: A1): A3 {
  console.println("foo78");
  return new A3();
}
