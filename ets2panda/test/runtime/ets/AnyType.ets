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

let a: Any

a = undefined
a = new Object()
a = null
a = 1

a = undefined as Any
a = new Object() as Any
a = null as Any
a = 1 as Any

function asgn<T = Any>(a: T): Any { return a }

arktest.assertTrue(asgn(null) === null)
arktest.assertTrue(asgn(undefined) === undefined)
arktest.assertTrue(asgn("a") === "a")
arktest.assertTrue(asgn(1) === 1)

class C<out T> {}

asgn<C<Any>>(new C<Any>())
asgn<C<Any>>(new C<null>())
asgn<C<Any>>(new C<number>())
asgn<C<Any>>(new C<string>())
asgn<C<Any>>(new C<never>())
