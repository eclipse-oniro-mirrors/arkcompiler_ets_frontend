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

interface It<T> {
}

class Cl<T> implements It<T> {

  constructor(p: T) {
    this.x = p;
  }  
 
  static resolve<U>(value: U|It<U>): Cl<U> {
    if (value instanceof Cl) {
        return value as Cl<U>;
    }
    return new Cl<U>(value as U);
  }

  x: T;

  print(): string {
    if (this.x == undefined) {
      return "value is " + this.x;
    }
    else if (this.x instanceof string) {
      return "string: '" + this.x + "'";
    } else {
      return "number = " + this.x;
    }
  }
}


function main(): void {

  arktest.assertEQ(Cl.resolve(undefined).print(), "value is undefined")
  arktest.assertEQ(Cl.resolve("test").print(), "string: 'test'")
  arktest.assertEQ(Cl.resolve(5.5).print(), "number = 5.5")
  arktest.assertEQ(Cl.resolve(new Int(8)).print(), "number = 8")

  arktest.assertEQ(Cl.resolve(new Cl<null>(null)).print(), "value is null")
  arktest.assertEQ(Cl.resolve(new Cl<string>("TEST")).print(), "string: 'TEST'")
  arktest.assertEQ(Cl.resolve(new Cl<number>(7.7)).print(), "number = 7.7")
  arktest.assertEQ(Cl.resolve(new Cl<Int>(new Int(-8))).print(), "number = -8")
}
