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

import { collections } from './oh_modules/@arkts.collections';

import { collections as collectionsAlias } from './oh_modules/@arkts.collections';

import { collections as kitCollections } from './oh_modules/@kit.ArkTS';

import { collections as definedCollections } from './ignore_files/user_defined_collections'; //legal

function tesCollectionsUsage() {

  const collections1: collections.Array<number> = new collections.Array<number>();

  const collections2 = new collectionsAlias.Array<number>();

  const collections3 = new kitCollections.Array<number>();

  let collections4: collectionsAlias.Array<number>;

  const collections5: collectionsAlias.Array<number> = new collectionsAlias.Array<number>();

  let collections6: definedCollections.Array<number>; // legal

}

function test(array: collections.Array<string>) {
    const map = collections.Map<string, string>();
}

function testBitVector(bv: collections.BitVector<string>) {
    const bitVector = new collections.BitVector<string>();
}
