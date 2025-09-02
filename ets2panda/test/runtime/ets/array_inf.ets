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

class A {
    foo(p: FixedArray<Object>): int {
        let s = 0
        for (let v of p) {
            s += v instanceof A ? 1 : 0
        }
        return s
    }
    bar(p: FixedArray<A>): int {
        let s = 0
        for (let v of p) {
            s += v instanceof A ? 1 : 0
        }
        return s
    }
  }
  
  
  class B extends A{}
  
  function main() {
    arktest.assertEQ(new A().foo([new Object(), new Long(), new Int()]), 0)
    arktest.assertEQ(new A().foo([new A(), new A(), new Object()]), 2)
    arktest.assertEQ(new A().foo(["aaa", new A(), new Object()]), 1)
    arktest.assertEQ(new A().foo([1, "hello", new A()]), 1)
    arktest.assertEQ(new A().bar([new B(), new B(), new A()]), 3)
  }
  