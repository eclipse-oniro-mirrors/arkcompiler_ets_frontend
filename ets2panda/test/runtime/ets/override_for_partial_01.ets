/* Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

abstract class A77<T> {
  abstract foo(x?: Partial<T>): string; 
}

class B77 extends A77<B77> {
 override foo(x?: Partial<B77>): string {
     if (x == undefined) {
       return "undefined";
     } else {
       return "Partial<B77>";
    }
  };
}

function bar<T>(x: A77<T>, y: Partial<A77<T>>): void {
   arktest.assertEQ(x.foo(undefined), "undefined")
   arktest.assertEQ(x.foo(y), "Partial<B77>")
}

function main() : void {
   let x: A77<B77>  = new B77();
   let y: Partial<B77> = {};

   arktest.assertEQ(x.foo(undefined), "undefined")
   arktest.assertEQ(x.foo(y), "Partial<B77>")

   bar<B77>(x, y);

   let z = new B77();
   arktest.assertEQ(z.foo(undefined), "undefined")
   arktest.assertEQ(z.foo(y), "Partial<B77>")
}
