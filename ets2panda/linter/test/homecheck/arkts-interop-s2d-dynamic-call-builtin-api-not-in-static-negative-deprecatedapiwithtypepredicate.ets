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

import { animals } from './fixtures/interop/s2d/builtin-api/exportEts2';
import { array11, array22 } from './fixtures/interop/s2d/builtin-api/exportEts1';
import { Animal, Dog } from './fixtures/interop/s2d/builtin-api/BaseClass';

function isDog(animal: Animal, index: number, array: Array<Animal>): animal is Dog {
    return animal instanceof Dog;
}

function testArrayEveryAPI(param1: number[], param2: number[]): void {
    animals.every(isDog, animals);        // 告警1，animals来自arkts1.2
    const arrayFromImport = animals;
    const array = arrayFromImport;
    array.every(isDog, animals);          // 告警2
    param1.every(isDog, animals);         // 告警3
    param2.every(isDog, animals);         // 告警4
}

testArrayEveryAPI(animals, [1, 2, 3]);
testArrayEveryAPI([1, 2, 3], animals);

function testArrayFilterAPI(): void {
    animals.filter(isDog, animals);       // 告警5
}

function isDogReadonly(animal: Animal, index: number, array: readonly Array<Animal>): animal is Dog {
    return animal instanceof Dog;
}

function testArrayFilterWithReadonlyAPI(): void {
    animals.filter(isDogReadonly, animals);       // 告警6
}

function isDogWithVoidThis(this: void, animal: Animal, index: number, obj: Array<Animal>): animal is Dog {
    return animal instanceof Dog;
}

function testArrayFindAPIWithVoidThis(): void {
    animals.filter(isDogWithVoidThis, animals);       // 告警7
}

function positiveArrayEveryAPI(): void {
    const array1 = [1, 2, 3];
    array1.every(isDog, animals);              // 不告警
    const array2 = array22;
    array2.every(isDog, animals);              // 不告警
    animals.every(a.compare);                  // 不告警
}

function positiveArrayFilterAPI(): void {
    array11.filter(isDog, animals);            // 不告警
    animals.filter(isDog);                     // 不告警
}

function positiveArrayFilterWithReadonlyAPI(): void {
    array11.filter(isDogReadonly, animals);    // 不告警
    animals.filter(isDogReadonly);             // 不告警
}

function positiveArrayFindAPIWithVoidThis(): void {
    array11.filter(isDogWithVoidThis, animals);    // 不告警
    animals.filter(isDogWithVoidThis);             // 不告警
}
