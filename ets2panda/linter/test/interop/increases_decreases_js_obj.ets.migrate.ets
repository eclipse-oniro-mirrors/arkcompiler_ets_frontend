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
import {foo} from "./increases_decreases_js_obj_js"
let a: number =0
a = foo.getProperty("num").toNumber()
foo.setProperty(num, a + 1)
a = a + 1

a = foo.getProperty("num").toNumber()
a = a + 1
foo.setProperty(num, a)

a = foo.getProperty("num").toNumber()
foo.setProperty(num, a - 1)
a = a - 1

a = foo.getProperty("num").toNumber()
a = a - 1
foo.setProperty(num, a)


let tmp_1 = foo.getProperty("num").toNumber()
foo.setProperty(num, tmp_1 + 1)
tmp_1 = tmp_1 + 1

let tmp_2 = foo.getProperty("num").toNumber()
tmp_2 = tmp_2 + 1
foo.setProperty(num, tmp_2)

let tmp_3 = foo.getProperty("num").toNumber()
foo.setProperty(num, tmp_3 - 1)
tmp_3 = tmp_3 - 1

let tmp_4 = foo.getProperty("num").toNumber()
tmp_4 = tmp_4 - 1
foo.setProperty(num, tmp_4)

