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

namespace A {
   function foo(): void {}
}

namespace A {
   function foo(): void {}  // Error
}

namespace B {
  class MyClass {} 
}

namespace B {
  class MyClass {}  // Error
}

namespace C {
  let myVar: number = 42; 
}

namespace C {
  let myVar: number = 42;  // Error
}

namespace D {
  let a: number = 1, c: number = 2;  
}

namespace D {
  let b: number = 3, c: number = 4;  // Error
}

namespace F {
  interface MyInterface {} 
}

namespace F {
  interface MyInterface {}  // Error
}

namespace G {
  enum MyEnum { A, B, C } 
}

namespace G {
  enum MyEnum { A, B, C }  // Error
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
    function foo2(): void {}  // Error
}

namespace Root {
  namespace Level1 {
    namespace Level2 {
      function foo() {}
    }
  }
}

namespace Root {
  namespace Level1 {
    namespace Level2 {
      function foo() {}  // Error
    }
  }
}
