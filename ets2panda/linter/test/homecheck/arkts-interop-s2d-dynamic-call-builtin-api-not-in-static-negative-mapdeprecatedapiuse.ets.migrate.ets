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

import { map1, foo3 } from './fixtures/interop/s2d/builtin-api/exportEts2';
import { map11, map4 } from './fixtures/interop/s2d/builtin-api/exportEts1';

class Ctx {
    log(key: number, value: string): void {
        console.log(key, value);
    }
}

function foo(value: string, key: string, map: Map<number, string>) {}

const mapGlobal1 = map1;
const mapGlobal2 = mapGlobal1;

function testMapForEachAPI(param1: Map<number, string>, param2: Map<number, string>): void {
    map1.forEach(foo, new Ctx());                       // 告警1
    const mapFromImport = map1;
    const map2 = mapFromImport;
    map2.forEach(foo, new Ctx());                       // 告警2
    mapGlobal2.forEach(foo, new Ctx());                 // 告警3
    const mapFromGlobal = mapGlobal1;
    mapFromGlobal.forEach(foo, new Ctx());              // 告警4
    const mapFromReturnValue = foo3();
    mapFromReturnValue.forEach(foo, new Ctx());         // 告警5
    param1.forEach(foo, new Ctx());                     // 告警6
    param2.forEach(foo, new Ctx());                     // 告警7
}

testMapForEachAPI(mapGlobal2, map11);
testMapForEachAPI(map11, map1);

let mapGlobal11 = map1;
mapGlobal11 = map11;

function positiveMapEveryAPI(): void {
    map11.forEach(foo, new Ctx());          // 不告警
    const map1 = new Map<number, string>;
    map1.forEach(foo, new Ctx());           // 不告警
    mapGlobal11.forEach(foo, new Ctx());    // 不告警
    mapGlobal1.forEach(foo);                // 不告警
}

namespace TestNS {
    class TestCrossMethod {
        map = this.test1();
        foo(value: string, key: string, map: Map<number, string>) {}

        test1(): Map<number, string> {
            return map1;
        }

        test2(): Map<number, string> {
            return this.map;
        }

        test3(): Map<number, string> {
            return this.test2();
        }

        testWithOneCallSite() {
            this.test1().forEach(foo, new Ctx());        // 告警8
        }

        testWithTwoCallSite() {
            this.test2().forEach(foo, new Ctx());        // 告警9
        }

        testWithThreeCallSite() {
            this.test3().forEach(this.foo, new Ctx());        // 告警10
        }
    }
}

function testMultipleImportMap(): void {
    map4.forEach(foo, new Ctx());                       // 告警11
}
