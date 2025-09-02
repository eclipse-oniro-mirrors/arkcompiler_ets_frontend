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

class A<T> {}
class B<T> {}

class C {
  foo<T, U>(...p: [A<T>, B<U>]): boolean {
      return p[0] == p[1]
  }
  
  moo<T, U>(a:int, ...p: [A<T>, B<U>]): boolean {
      return p[0] == p[1]
  }
}

function main() {
    let a1: [A<Long>, B<Error>] = [new A<Long>, new B<Error>]
    
    arktest.assertTrue((new C()).foo<Long, Error>(...a1) == false)
    arktest.assertTrue((new C()).moo<Long, Error>(12, ...a1) == false)

    arktest.assertTrue((new C()).foo<Long, Error>(...[new A<Long>, new B<Error>]) == false)
    arktest.assertTrue((new C()).moo<Long, Error>(12, ...[new A<Long>, new B<Error>]) == false)

    arktest.assertTrue((new C()).foo<Long, Error>(...[new A<Long>, new B<Error>] as [A<Long>, B<Error>]) == false)
    arktest.assertTrue((new C()).moo<Long, Error>(12, ...[new A<Long>, new B<Error>] as [A<Long>, B<Error>]) == false)
    
    arktest.assertTrue((new C()).foo<Long, Error>(new A<Long>, new B<Error>) == false)
    arktest.assertTrue((new C()).moo<Long, Error>(12, new A<Long>, new B<Error>) == false)
}
