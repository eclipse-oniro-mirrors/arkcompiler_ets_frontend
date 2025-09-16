/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

import { dummy, abc, Foo  } from "./findRenameLocations1.ets";

dummy(4);
dummy(44);
abc(5);
abc(55);
abc(555);

let myfoo = new Foo("apples", 1, 2, 3);
let otherfoo = new Foo("oranges", 4, 5, 6);

console.log(myfoo)
console.log(otherfoo)
console.log(myfoo.name)
