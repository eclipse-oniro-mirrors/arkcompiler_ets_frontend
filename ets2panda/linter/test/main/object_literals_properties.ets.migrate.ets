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

// Untyped object literals
class GeneratedObjectLiteralClass_1 {
    m() { console.log(1); } // Error, fixable
}

let method = new GeneratedObjectLiteralClass_1();

class GeneratedObjectLiteralClass_2 {
    get property() { return 2; } // Error, fixable
}

let getMethod = new GeneratedObjectLiteralClass_2();

class GeneratedObjectLiteralClass_3 {
    set property(value: number) {
        console.log(value);
    }
}

let setMethod = new GeneratedObjectLiteralClass_3();

let x = 1, y = '2', z = true;

let shorthand = {
  x, // Error, fixable
  y, // Error, fixable
  z // Error, fixable
};

let spread = {
  ...shorthand // Error, not fixable
};

class GeneratedObjectLiteralClass_4 {
    a: string;
    b: number;
    c: number[];
    x: number;
    y: string;
    constructor(init: GeneratedObjectLiteralInitInterface_4) {
        this.a = init.a;
        this.b = init.b;
        this.c = init.c;
        this.x = init.x;
        this.y = init.y;
    }
    method() {
        console.log(42);
    }
    get property() {
        return 0;
    }
    set property(value: number) {
        if (value < 0) {
            throw new Error('Bad value');
        }
    }
}

interface GeneratedObjectLiteralInitInterface_4 {
    a: string;
    b: number;
    c: number[];
    x: number;
    y: string;
}

let mixed = new GeneratedObjectLiteralClass_4({
    a: "foo",
    b: 42,
    c: [1, 2, 3],
    x: x,
    y: y
});

let x2 = 1, y2 = 2, z2 = 3;
let mixedBad = { // Not fixable
  a: 1,
  b: 2,
  x2, // Error, fixable
  y2, // Error, fixable
  z2, // Error, fixable
  m() {},
  ...shorthand // Error, not fixable
}

// Typed object literals
interface I {
  m(): void;
}
let i: I = {
  m() { // Fixable
    console.log(100);
  }
};

class C {
  m(): void {
    console.log(200);
  }
}
let c: C = {
  m(): void { // Fixable
    console.log(300);
  }
};

function foo(c: C) {}
foo({
  m() { console.log(300); } // Fixable
});

class C2 {
  x2 = 10;
  y2 = 20;
  z2 = 30;

  m() {}
}
let c2: C2 = {
  x2, // Fixable
  y2, // Fixable
  z2, // Fixable
  m() { console.log(1); } // Fixable
};

let c22: C2 = {
  x2, // Fixable
  y2, // Fixable
  z2, // Fixable
  m() { console.log(1); }, // Not fixable, object has spread property
  ...shorthand // Not fixable
};

class C3 {
  x2 = 10;
  y2 = 20;
  z2 = 30;

  m() {}

  constructor(a: number) {}
}
let c3: C3 = {
  x2, // Fixable
  y2, // Fixable
  z2, // Fixable
  m() { console.log(1); } // Not fixable, class type has constructor with parameters
};

function capturesFromLocalScope() {
  let a = 1, b = 2;
  let captureLocalVal = {
    m() { // Not fixable, captures local values 'a' and 'b'
      console.log(a, b);
    }
  };

  let captureLocalVal2: C = {
    m(): void { // Not fixable, captures local values 'a' and 'b'
      console.log(a, b);
    }
  };

  interface LocalType {
    a: number;
    b: string;
}
  let localTypeVar: LocalType = { a: 1, b: '2' };
  let captureLocalType = {
    m() { // Not fixable, captures value of type `LocalType` declared in local scope
      console.log(localTypeVar);
    }
  };
  let captureLocalType2 = {
    m(x: LocalType) { // Not fixable, `x` references type `LocalType` declared in local scope
      console.log(x);
    }
  };

  class LocalClass { x: number = 1 };
  let captureLocalType3 = {
    m() { // Not fixable, references type `LocalClass` declared in local scope
      console.log(new LocalClass());
    }
  };
}

// Method overriding field
class C4 {
  a: number = 0;
  b() {};
}
let c4: C4 = { // Not fixable, overrides class method with property of functional type
  a: 1,
  b: () => {}
};

class C5 {
  a: number = 0;
  b: () => void;
}
let c5: C5 = { // Not fixable, overrides class property with method
  a: 1,
  b() {}
};

interface I2 {
  a: number;
  b(): void;
}
let i2: I2 = { // Not fixable, implements method as functional-type property
  a: 1,
  b: () => {}
};

interface I3 {
  a: number;
  b: () => void;
}
let ii: I3 = { // Not fixable, implements functional-type property as a method
  a: 1,
  b() {}
};

// Inheritance
class Base {
  constructor() {}
}
class Derived extends Base {
  m() {}
}
let b: Derived = { // Fixable
  m() { console.log(2); }
};

class Base2 {
  constructor(a: number) {}
}
class Derived2 extends Base2 {
  m() {}
}
let b2: Derived2 = { // Not fixable, derived class inherits a constructor with parameters from base class
  m() { console.log(2); }
};

class Base3 {
  constructor(a: number) {}
}
class Derived3 extends Base3 {
  m() {}

  constructor() {
    super(1);
  }
}
let b3: Derived3 = { // Fixable
  m() { console.log(2); }
};

interface I4 {
  map: Map<string, string>;
}
let map:Map<string,string> = new Map<string,string>();
let i4: I4 = {map};

class C6 {
  map1: Map<string, string> = new Map<string,string>();
}

let map1:Map<string,string> = new Map<string,string>();
let c6: C6 = {map1};

// Namespace typed object literals
namespace X {
  export class C {
    m() {
      console.log("C - 1");
    }
  }

  export interface I {
    m(a: number, b: string): void;
  }
}

function test() {
  let c: X.C = {
    m() {
      console.log("C - 2");
    }
  }

  let i: X.I = {
    m(): void {
      console.log("I");
    }
  }
}

class FooBarBaz {
    foo?: Map<string, Object>
    bar?: string
}

function baz(fooBar: Map<string, Object>) {
    baz2({fooBar});
}

function baz2(fooBarBaz: FooBarBaz) {
}

class JJ {
    mm?: Map<string, Object>
    aa?: string
}
const mm: Map<string,Object> = new Map<string, Object>();
let nn: JJ = {mm} // no error


class DD {
    tt?: Map<number, Object>
    gg?: string
}
const tt: Map<string,Object> = new Map<string, Object>();
let gg: DD = {tt} // error

interface Q {
    a:number;
    foo():void;
}

let w: Q = {
    a:1.0,
    foo(){} // no error
}