/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


import { goo, foo } from './fixtures/homecheck-interop-backward-dfa/TSObjectAPI_1.2obj/ets/file1'

class X { a = 1 }

goo(new X())    // 在此报错

const objX2 = new X()
goo(objX2)  // 在此报错

foo(new X())    // 在此报错

const objX = new X()
foo(objX)   // 在此报错