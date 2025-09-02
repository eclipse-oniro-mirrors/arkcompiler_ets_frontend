/*
* Copyright (c) 2025 Huawei Device Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable low or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

class A<T> {}
class B<T> {}

class C<T, U> {
  public field : boolean

  constructor(...p: [A<T>, B<U>]) {
    this.field = p[0] == p[1]
  }

  constructor(a: int, ...p: [A<T>, B<U>]) {
    this.field = p[0] == p[1]
  }
}

function main() {
  /* @@ label1 */new C<Long, Error>
  /* @@ label2 */new C<Long, Error>(new A<Long>)
  /* @@ label3 */new C<Long, Error>(new A<Long>, new B<Error>, new B<Error>)
}

/* @@@ label1 Error TypeError: Expected 1 arguments, got 0. */
/* @@@ label1 Error TypeError: Expected 2 arguments, got 0. */
/* @@@ label1 Error TypeError: Expected 3 arguments, got 0. */
/* @@@ label1 Error TypeError: No matching construct signature */
/* @@@ label2 Error TypeError: Expected 2 arguments, got 1. */
/* @@@ label2 Error TypeError: Expected 3 arguments, got 1. */
/* @@@ label2 Error TypeError: No matching construct signature for rest_parameter_13.C(A<Long>) */
/* @@? 33:37 Error TypeError: Type 'A<Long>' is not compatible with type 'Int' at index 1 */
/* @@@ label3 Error TypeError: Expected 2 arguments, got 3. */
/* @@@ label3 Error TypeError: No matching construct signature for rest_parameter_13.C(A<Long>, B<Error>, B<Error>) */
/* @@? 34:37 Error TypeError: Type 'A<Long>' is not compatible with type 'Int' at index 1 */
