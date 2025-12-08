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

namespace ATest {
  function foo() {}

  namespace BTest {
    function foo() {}
    function foo1() {}
  }

  function foo2() {}
}


namespace Ax {
   function foo1(): void {}
}

namespace Ax {
   function foo2(): void {}
}

namespace Cx {
   function foo2(): void {}
}

namespace Cx {
   function foo3(): void {}
}

namespace B {
  class MyClass {} // OK
}

namespace B {
  class MyClass {} // REPORT NO ERROR
}

namespace C {
  let myVar: number = 42; // OK
}

namespace C {
  let myVar: number = 42; // REPORT NO ERROR
}

namespace D {
  let y: number = 1, z: number = 2; // OK
}

namespace F {
  interface MyInterface {} // OK
}

namespace F {
  interface MyInterface {} // REPORT NO ERROR
}

namespace G {
  enum MyEnum { A, B, C } // OK
}

namespace G {
  enum MyEnum { A, B, C } // REPORT NO ERROR
}

namespace H {
  let a: number = 1, b: number = 2; // OK
}

namespace H {
  let c: number = 3, d: number = 4; // OK
}

namespace I {
  let a: number = 1, c: number = 2;
}

namespace I {
  let b: number = 3, c: number = 4; // REPORT NO ERROR
}


namespace Atest {
   function foo1(): void {}
}

namespace Btest {
    namespace Atest {
        function foo1(): void {}
    }

    function foo2(): void {}
}

namespace Btest {
    function foo3(): void {}
}

namespace Root {
  namespace Level1 {
    namespace Level2 {
      function compute() {}
      function compute2() {}
    }
  }
}
