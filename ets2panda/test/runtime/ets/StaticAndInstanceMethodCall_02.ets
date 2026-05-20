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

class A {}

let m: Map<int, string> = new Map<int, string>();

m.set(1, "1");
let s: int | string | number | undefined | A = m.size;
let x = m.size.toString(32);
arktest.assertEQ(s!.toString(), x);

m.set(2, "2");
s = m.size;  
x = m.size.toString(32);
arktest.assertEQ(s!.toString(), x);


let a: Array<int> = [];

a.push(1);
s = a.length;
x = a.length.toString(32);
arktest.assertEQ(s!.toString(), x);

a.push(2);
s = a.length;
x = a.length.toString(32);
arktest.assertEQ(s!.toString(), x);
