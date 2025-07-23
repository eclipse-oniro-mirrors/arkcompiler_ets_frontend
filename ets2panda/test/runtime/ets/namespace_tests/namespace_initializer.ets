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

class A {
    public data: string;
    public constructor(d: string) {
        this.data = d;
    }
}
namespace NS1{
  export function foo():number {return 2}
  export let data:number;
  export let a: A;
  export let b: string;
  export let c: string = "immediate Initializer First!";
  static {
    a = new A("hello initializer block!");
    b = "hi initializer block";
    c = c + " initializer Block Second!";
    data = foo()
  }
}

arktest.assertEQ(NS1.data, 2);
arktest.assertEQ(NS1.a.data, "hello initializer block!");
arktest.assertEQ(NS1.b, "hi initializer block");
arktest.assertEQ(NS1.c, "immediate Initializer First! initializer Block Second!");

namespace NS2{
  export function foo():number {return 1}
  export let b: number;
  export let a: A;
  static {
     b = 1;
     a = new A("hello initializer block!")
  }
  
  export namespace NS3 {
      export let bb:number;
      export let aa: A;
      static {
        bb = 2;
        aa = new A("hello sub initializer block!")
    }
  }
}

arktest.assertEQ(NS2.b, 1);
arktest.assertEQ(NS2.a.data, "hello initializer block!");
arktest.assertEQ(NS2.NS3.bb, 2);
arktest.assertEQ(NS2.NS3.aa.data, "hello sub initializer block!");