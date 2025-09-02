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

class A{
  field1:number = 10;
  foo(){}
}

class B extends A{
  bfield1:number = 10;
  bfoo(){}
}

type keyofB = keyof B;

function main() : void {
  let c1:keyof B = "field1"
  let c2:keyof B = "foo"
  let c3:keyof B = "bfield1"
  let c4:keyof B = "bfoo"

  let c5:keyofB = "field1"
  let c6:keyofB = "foo"
  let c7:keyofB = "bfield1"
  let c8:keyofB = "bfoo"
}
