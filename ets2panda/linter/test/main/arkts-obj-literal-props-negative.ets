/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

interface property {
  property: string;
}

let getMethod: property = {
  get property() { return '2'; } // Object pattern can't contain methods.
};

let setMethod:property = {
  set property(value: string) { // Object pattern can't contain methods.
    console.log(value);
  }
};

interface xyz {
  x:number
  y:string
  z:boolean
}
let x = 1, y = '2', z = true;

let shorthand: xyz = {
  x,
  y,
  z
};
interface IMixed {
  a:string
  b:number
  c:number[]
  x: number
  y: string

  property:number
}
let mixed: IMixed = {
  a: "foo",
  b: 42,
  c: [1, 2, 3],
  x,
  y,

  get property() { // Object pattern can't contain methods.
    return 0;
  },

  set property(value: number) { // Object pattern can't contain methods.
    if (value < 0) {
      throw new Error('Bad value');
    }
  }
}

class A {
  foo: () => void = () => {}
}

let a: A = {
  foo() { // Incorrect method definition
    console.log('hello')
  }
}

interface Person {
  sayHello: () => void;
}

let p: Person = {
  sayHello() {  // Incorrect method definition
    console.log('Hi');
  }
};