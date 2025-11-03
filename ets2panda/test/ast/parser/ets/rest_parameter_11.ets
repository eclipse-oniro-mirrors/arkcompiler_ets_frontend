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

class C {
  foo<T, U>(...p: [A<T>, B<U>]) {}
}

/* @@ label */(new C()).foo(new A<Long>, new B<Error>, new B<Error>)

/* @@? 23:15 Error TypeError: No matching call signature for foo(A<Long>, B<Error>, B<Error>) */