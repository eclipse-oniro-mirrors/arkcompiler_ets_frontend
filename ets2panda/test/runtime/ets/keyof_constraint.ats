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
  field1:int;
  field2:int;
  method1(){};
}

class C<T extends keyof A>{}

function getProperty<T, K extends keyof A>(obj: T, key: K){}

function main():void{
  let c1 = new C<"field1">;
  let c2 = new C<"field2">;
  let c3 = new C<"method1">;
  let c4 = new C<keyof A>;

  let a = new A();
  getProperty<A, keyof A>(a, "field1");
  getProperty<A, keyof A>(a, "field2");
  getProperty<A, keyof A>(a, "method1");
}
