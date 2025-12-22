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

class Base {}
class Derived extends Base { }

function fooBB<T>(p: T): T { return p };
function fooBD<T, G extends T>(p: T): G { return p as G };

function main(): void {
    let foobb = fooBB<Base>;
    let foob_test1 = foobb(new Base())
    let foob_test2 = foobb(new Derived())

    let foobd = fooBD<Base, Derived>

    foobb = foobd

    try {
        foobb(new Base())
        arktest.assertTrue(false);
    } catch (e) {
        if(!(e instanceof ClassCastError)) {
            arktest.assertTrue(false);
        }
    }

    try {
        foobd(new Base())
        arktest.assertTrue(false);
    } catch (e) {
        if(!(e instanceof ClassCastError)) {
            arktest.assertTrue(false);
        }
    }
}
