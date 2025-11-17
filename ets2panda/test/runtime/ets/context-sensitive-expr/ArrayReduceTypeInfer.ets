/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

type tst = number | object
let cb = (p1: tst, p2: tst, idx: int, obj: Array<tst>): tst => { return new Object(); }
let init: tst = 5.5
let arr = new Array<tst>(1,2,3)
arr.reduce(cb, init)
arktest.assertTrue(true);
