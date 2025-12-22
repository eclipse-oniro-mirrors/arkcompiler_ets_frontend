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

namespace NS {
    export interface ReadonlyArray<T> {
      index:number;
    }
}

namespace NS2 {
    export class ReadonlyArray<T> {
      index:number = 10;
    }
}

function main(){
  let x1:NS.ReadonlyArray<number> = {index:10};
  arktest.assertEQ(x1.index,10);

  let x2 = new NS2.ReadonlyArray<number>();
  arktest.assertEQ(x2.index,10)
}
