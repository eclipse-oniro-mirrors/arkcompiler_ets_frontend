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

import { UsedImport } from './getSemanticDiagnosticsUnusedSource'
import * as UnusedModule from './getSemanticDiagnosticsUnusedSource'

let unusedTopLevelValue: number = 1
let unusedTopLevelArrow = () => {}
let usedTopLevelArrow = () => {
  return 1
}
export let exportedTopLevelArrow: () => void = () => {}

class UnusedClass {}
function unusedFunction(): void {}
enum UnusedEnum {
  VALUE,
}
interface UnusedInterface {}
type UnusedTypeAlias = number

class UsedAsBaseClass {}
class DerivedFromUsedAsBaseClass extends UsedAsBaseClass {
  value(): number {
    return 1
  }
}
interface UsedAsParentInterface {}
interface DerivedFromUsedAsParentInterface extends UsedAsParentInterface {}
type UsedTypeAlias = number
function usesTypeAlias(value: UsedTypeAlias): UsedTypeAlias {
  return value
}

function localParameterAndForCases(
  unusedParam: number,
  writtenOnlyParam: number,
  usedParam: number,
  ...unusedRest: number[]
): number {
  let usedLocal = usedParam + 1
  let unusedLocal = 42
  let writtenOnlyLocal = 0
  let total = 0

  writtenOnlyParam = 1
  writtenOnlyLocal = 2

  for (let unusedFor = 0; usedLocal < 3; usedLocal++) {
  }

  for (let usedFor = 0; usedFor < 2; usedFor++) {
    total += usedFor
  }

  return usedLocal + total
}

class PrivateMemberCases {
  private unusedField: number = 1
  private writtenOnlyField: number = 2
  private usedField: number = 3

  private unusedMethod(): number {
    return 1
  }

  private usedMethod(): number {
    return 2
  }

  test(): number {
    this.writtenOnlyField = 4
    return this.usedField + this.usedMethod()
  }
}

class SamePrivateFieldA {
  private value: number = 1

  test(): number {
    return this.value
  }
}

class SamePrivateFieldB {
  private value: number = 2

  test(): number {
    return 1
  }
}

class SamePrivateMethodA {
  private helper(): number {
    return 1
  }

  test(): number {
    return this.helper()
  }
}

class SamePrivateMethodB {
  private helper(): number {
    return 2
  }

  test(): number {
    return 1
  }
}

namespace NamespaceA {
  let value: number = 1
  let usedByLocalInitializer: number = value
}

namespace NamespaceB {
  let value: number = 1
}

function useImport(): UsedImport {
  return new UsedImport()
}

function useDeclarations(): number {
  let privateCases = new PrivateMemberCases()
  let sameFieldA = new SamePrivateFieldA()
  let sameFieldB = new SamePrivateFieldB()
  let sameMethodA = new SamePrivateMethodA()
  let sameMethodB = new SamePrivateMethodB()
  let derived = new DerivedFromUsedAsBaseClass()
  return usedTopLevelArrow() + derived.value() + usesTypeAlias(1) + localParameterAndForCases(1, 2, 3) +
    privateCases.test() + sameFieldA.test() + sameFieldB.test() + sameMethodA.test() + sameMethodB.test()
}

useImport()
useDeclarations()
