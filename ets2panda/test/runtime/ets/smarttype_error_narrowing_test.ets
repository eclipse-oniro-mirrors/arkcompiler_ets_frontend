/**
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at*
 *
 * http://www.apache.org/licenses/LICENSE-2.0*
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

class A {}
let a = new A;
let expects:Array<string> = ["A", "Object"];
function Test(expects:Array<string>) {
    let index:int = 0;
    for (let cls: Class|undefined = Class.of(a); cls !== undefined; cls = cls!.getSuper()) {
        if (cls !== undefined) {
            arktest.assertTrue(cls.getName().contains(expects[index], 0));
            index += 1;
        }
    }
}
Test(expects);