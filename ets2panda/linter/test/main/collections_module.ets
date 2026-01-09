/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

// Large Scenario Ⅰ: collections declaration file comes from SDK
import { collections } from './oh_modules/@kit.ArkTS';           // No need to check, [arkts-no-need-stdlib-sendable-containers]
import { collections as col1 } from './oh_modules/@kit.ArkTS';   // No need to check, [arkts-no-need-stdlib-sendable-containers]
import { collections as col2 } from './oh_modules/@kit.ArkTS';   // No need to check, [arkts-no-need-stdlib-sendable-containers]
import { collections as col3 } from './oh_modules/@kit.ArkTS';   // No need to check, [arkts-no-need-stdlib-sendable-containers]
import { collections as col4 } from './oh_modules/@kit.ArkTS';   // Need to check and auto-fix, (arkts-no-need-stdlib-sendable-containers)
import { collections as col5 } from './oh_modules/@kit.ArkTS';   // Need to check but not auto-fix, (arkts-no-need-stdlib-sendable-containers) 
import { collections as col6 } from './oh_modules/@kit.ArkTS';   // Need to check and auto-fix, (arkts-no-need-stdlib-sendable-containers)

// Scenario 1: Only the BitVector container was used
// Import path need to fixed using '@arkts.collections' by the developer manually.
let bitVector1: collections.BitVector = new collections.BitVector(0);  // No need to check, [arkts-no-need-stdlib-sendable-containers]

// Scenario 2: Both BitVector and non-BitVector containers exist
// Now depends on whether Array constructor was used with length argument

// Scenario 2.1: Array was used, and constructor call was not with length
// Need to check and auto-fix for Array
let bitVector2: col1.BitVector = new col1.BitVector(0);     // No need to check, [arkts-no-need-stdlib-sendable-containers]
let array1: col1.Array<number> = new col1.Array<number>(); // no-argument scenario, both places (arkts-no-need-stdlib-sendable-containers)
let arr: string[] = ["aa", "bb"];
let array2: col1.Array<string> = new col1.Array<string>(...arr);  // array argument scenario, both places (arkts-no-need-stdlib-sendable-containers)

// Scenario 2.2: Array was used, and constructor call was with length
// Need to check but not to auto-fix for Array
let bitVector3: col2.BitVector = new col2.BitVector(0);   // No need to check, [arkts-no-need-stdlib-sendable-containers]
let array3: col2.Array<number> = new col2.Array<number>(5); // both places (arkts-no-need-stdlib-sendable-containers), but no need to be auto-fixed

// Scenario 2.3: Array was not used, other containers were used
// Need to check and auto-fix for Non-Array
let bitVector4: col3.BitVector = new col3.BitVector(0);                  // No need to check, [arkts-no-need-stdlib-sendable-containers]
let map1: col3.Map<string, boolean> = new col3.Map<string, boolean>();  // Non-Array scenario, both places (arkts-no-need-stdlib-sendable-containers)

// Scenario 3: BitVector container does not exist, non-BitVector containers were used

// Scenario 3.1: Array was used, and constructor call was not with length
// Need to check and auto-fix for Array
let array4: col4.Array<number> = new col4.Array<number>();   // both places (arkts-no-need-stdlib-sendable-containers)

// Scenario 3.2: Array was used, and constructor call was with length
// Need to check but not to auto-fix for Array
let array5: col5.Array<number> = new col5.Array<number>(5);  // both places (arkts-no-need-stdlib-sendable-containers), but no need to be auto-fixed

// Scenario 3.3: Array was not used, other containers were used
// Need to check and auto-fix for Array
let map2: col6.Map<string, boolean> = new col6.Map<string, boolean>();  // both places (arkts-no-need-stdlib-sendable-containers)




// Large Scenario Ⅱ: collections declaration file comes from user-defined
// The following codes no need to be checked and auto-fixed
import { collections as definedCol } from './ignore_files/user_defined_collections';
import { collections as definedCol2 } from './ignore_files/user_defined_collections';
import { collections as definedCol3 } from './ignore_files/user_defined_collections';
import { collections as definedCol4 } from './ignore_files/user_defined_collections';

let bitVector5: definedCol.BitVector = new definedCol.BitVector(0);
let array6 : definedCol.Array<number> = new definedCol.Array<number>(0);

let bitVector6: definedCol2.BitVector = new definedCol2.BitVector(0);
let array7 : definedCol2.Array<number> = new definedCol2.Array<number>();

let array8 : definedCol3.Array<number> = new definedCol3.Array<number>(0);

let array9 : definedCol4.Array<number> = new definedCol4.Array<number>();



// Large Scenario Ⅲ: collections declaration file cannot be found
// The following codes no need to be checked and auto-fixed
import { collections as notFoundCol } from './ignore_files/notFound_collections';
import { collections as notFoundCol2 } from './ignore_files/notFound_collections';
import { collections as notFoundCol3 } from './ignore_files/notFound_collections';
import { collections as notFoundCol4 } from './ignore_files/notFound_collections';

let bitVector7: notFoundCol.BitVector = new notFoundCol.BitVector(0);
let array10 : notFoundCol.Array<number> = new notFoundCol.Array<number>(0);

let bitVector8: notFoundCol2.BitVector = new notFoundCol2.BitVector(0);
let array11 : notFoundCol2.Array<number> = new notFoundCol2.Array<number>();

let array12 : notFoundCol3.Array<number> = new notFoundCol3.Array<number>(0);

let array13 : notFoundCol4.Array<number> = new notFoundCol4.Array<number>();