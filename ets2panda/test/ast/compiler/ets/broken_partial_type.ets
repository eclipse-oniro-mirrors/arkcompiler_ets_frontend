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

enum Kind {
  A = "A",
  B = "B"
}

interface Entity {
  kind: () => Kind
}

interface EntityA extends Entity {
  kind: () => Kind.A
}

interface EntityB extends Entity {
  kind: () => Kind.B
}

type EntityUnion = EntityA | EntityB

function createEntityA(): EntityUnion {
  const entity: Partial<EntityA> = {}
  return entity as EntityUnion
}

/* @@? 25:1 Error Semantic error ESE0070: 'A' type does not exist. */
/* @@? 25:1 Error Semantic error ESE0136: Method kind(): ((() => *ERROR_TYPE*))|undefined in Partial<EntityA> not overriding any method */
/* @@? 25:1 Error Semantic error ESE985118: Class member kind with type ((() => *ERROR_TYPE*))|undefined in Partial<EntityA> cannot implement kind with type ((() => Kind))|undefined property from interface Partial<Entity> because the implementing return type is not compatible with the interface return type. */
/* @@? 26:3 Error Semantic error ESE0070: 'A' type does not exist. */
/* @@? 26:3 Error Semantic error ESE0136: Method kind(): (() => *ERROR_TYPE*) in EntityA not overriding any method */
/* @@? 26:3 Error Semantic error ESE985118: Class member kind with type (() => *ERROR_TYPE*) in EntityA cannot implement kind with type (() => Kind) property from interface Entity because the implementing return type is not compatible with the interface return type. */
/* @@? 30:3 Error Semantic error ESE0070: 'B' type does not exist. */
/* @@? 30:3 Error Semantic error ESE0136: Method kind(): (() => *ERROR_TYPE*) in EntityB not overriding any method */
/* @@? 30:3 Error Semantic error ESE985118: Class member kind with type (() => *ERROR_TYPE*) in EntityB cannot implement kind with type (() => Kind) property from interface Entity because the implementing return type is not compatible with the interface return type. */

