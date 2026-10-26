/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { InterfaceAA,  ClassAA, getInterfaceAA} from './fixtures/homecheck-observed-decorator/ets/ClassDeclareEts'
import { ClassCC, getClassAA } from './fixtures/homecheck-observed-decorator/ets/ClassDeclareTs'

let cond = true

@Entry
@Component
struct MyComponent {
  @State obj = new ClassAA();      // @Observed ClassAA from ets
  @State obj1 = cond ? new ClassCC() : {};  // @Observed ClassAA, ClassBB, ClassCC from ts
  @State obj2: ClassCC = {a: 1};   // @Observed ClassAA, ClassBB, ClassCC from ts
  @State obj3 = getClassAA();    // @Observed ClassBB, ClassAA from ts
  @State obj4: InterfaceAA = new ClassAA(); // @Observed ClassAA from ets
  @State obj5: InterfaceAA = cond ? {} : new ClassAA();    // @Observed ClassAA from ets
  @State obj6: InterfaceAA = getInterfaceAA();  // @Observed ClassCC, ClassBB, ClassAA from ets

  build() {
  }
}