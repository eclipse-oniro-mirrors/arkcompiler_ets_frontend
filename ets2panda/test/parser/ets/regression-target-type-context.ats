/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

class Bar {
    constructor(arg0: double, arg1: double, arg2: double, arg3: double, arg4: double, arg5: double, arg6: double) {
    }

    foo(arg0: double, arg1: double, arg2: double, arg3: double, arg4: double, arg5: double, arg6: double) : boolean {
      return false;
    }

    foo2() : void {
      let x : boolean = this.foo(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0);
    }

    getOne() : double {
      return 1.0;
    }
}

function foo3() : Bar {
  return new Bar(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0);
}

function foo4(): double {
  let ret : double = 2.0;
  let BarInstance : Bar = new Bar(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0);
  ret += BarInstance.getOne();
  return ret;
}
