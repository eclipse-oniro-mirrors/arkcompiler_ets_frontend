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

//Note: validate public method could be used as value.
function doTaskOutSide(doSomething: ()=>void) {
    doSomething();
}

class A {
  data: string = "hello"
  method():void {
    this.data = "hello in method"
  }
  doTask(doSomething: ()=>void) {
      doSomething();
  }
}

let a = new A();
a.doTask(a.method);
assertEQ(a.data, "hello in method");
a.data = "";
assertEQ(a.data, "");
doTaskOutSide(a.method);
assertEQ(a.data, "hello in method");
