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

let factorial: (n: int) => number; // arkts-numeric-semantic
factorial = (n: int): number => { // arkts-numeric-semantic
  if (n <= 1) { return 1; }
  return n * factorial(n - 1);
};
// Test for NumericSignatureMatcher: Function type signature comparison

type NumericFunctionType = (n: int) => number // Function type alias

function functionTypeTest(func: NumericFunctionType): void { // arkts-numeric-semantic - function type param
  const result = func(100)
  const arr: number[] = new Array(result)
}

function functionTypeReturn(): NumericFunctionType { // arkts-numeric-semantic - function type return
  return (n: int) => n + 50 // arkts-numeric-semantic
}

function callbackSignatureTest(callback: (value: number) => void): void { // arkts-numeric-semantic - callback signature
  callback(200)
}

function nestedFunctionTypeTest(): void {
  const outerFunc = (outer: number): ((inner: number) => number) => { // arkts-numeric-semantic - nested function type
    return (inner: number) => outer + inner // arkts-numeric-semantic
  }
  
  const innerFunc = outerFunc(300)
  const result = innerFunc(400)
  const arr: number[] = new Array(result)
}

interface ObjectWithFunction {
  processor: (n: int) => number
}

function functionTypeInObject(): void {
  const obj: ObjectWithFunction = {
    processor: (n: int) => n + 500 // arkts-numeric-semantic
  }
  
  const result = obj.processor(600)
  const arr: number[] = new Array(result)
}

// Scenario: type alias shared between int-compatible and number-required usages

type SharedFunctionType = (n: number) => number

function sharedFunctionIntUsage(): SharedFunctionType {
  return (n: number) => n + 50 // n only used as int (addition + return)
}

function sharedFunctionNumberUsage(): SharedFunctionType {
  return (n: number) => n / 2.0 // n used in division → number context, should NOT fix
}

// Scenario: interface field shared between int-compatible and number-required usages

interface SharedInterface {
  processor: (n: number) => number
}

function sharedInterfaceIntUsage(): void {
  const obj: SharedInterface = {
    processor: (n: number) => n + 500 // n only used as int
  }
  const result = obj.processor(600)
  const arr: number[] = new Array(result)
}

function sharedInterfaceNumberUsage(): void {
  const obj: SharedInterface = {
    processor: (n: number) => n / 2.0 // n used in division → should NOT fix
  }
  const result = obj.processor(3.14)
  console.log(result)
}

// Scenario: exported type alias and interface → cross-file risk → should NOT fix

export type ExportedFunctionType = (n: number) => number

function exportedFunctionIntUsage(): ExportedFunctionType {
  return (n: number) => n + 50 // n only used as int, but type alias is exported → should NOT fix
}

export interface ExportedInterface {
  processor: (n: number) => number
}

function exportedInterfaceIntUsage(): void {
  const obj: ExportedInterface = {
    processor: (n: number) => n + 500 // n only used as int, but interface is exported → should NOT fix
  }
  const result = obj.processor(600)
  const arr: number[] = new Array(result)
}

// Scenario: non-exported class with function type field → should fix (companion fix on class field)
//           exported class with function type field → should NOT fix

class NonExportedClassWithFnField {
  handler: (n: int) => number = (n: int) => n + 50 // n only used as int → should fix both
}

export class ExportedClassWithFnField {
  handler: (n: number) => number = (n: number) => n + 50 // class exported → should NOT fix
}

// Scenario: object literal typed by class → companion fix on class field

class NonExportedClassWithProp {
  processor: (n: int) => number
}

function classPropertyIntUsage(): void {
  const obj: NonExportedClassWithProp = {
    processor: (n: int) => n + 500 // n only used as int, class not exported → should fix
  }
  const result = obj.processor(600)
  const arr: number[] = new Array(result)
}

export class ExportedClassWithProp {
  processor: (n: number) => number
}

function exportedClassPropertyIntUsage(): void {
  const obj: ExportedClassWithProp = {
    processor: (n: number) => n + 500 // class exported → should NOT fix
  }
  const result = obj.processor(600)
  const arr: number[] = new Array(result)
}

// Scenario: exported class method return type with function type → should NOT fix

export class ExportedClassWithMethod {
  getHandler(): (n: number) => number {
    return (n: number) => n + 50 // class exported → should NOT fix
  }
}

class NonExportedClassWithMethod {
  getHandler(): (n: int) => number {
    return (n: int) => n + 50 // class not exported → should fix (companion on return type)
  }
}

// Scenario: non-exported type alias used in exported function → should NOT fix

type NonExportedTypeInExportedFunc = (n: number) => number

export function exportedFuncUsingTypeAlias(): NonExportedTypeInExportedFunc {
  return (n: number) => n + 50 // function exported → should NOT fix
}

// Scenario: non-exported interface used in exported function → should NOT fix

interface NonExportedInterfaceInExportedFunc {
  handler: (n: number) => number
}

export function exportedFuncUsingInterface(): NonExportedInterfaceInExportedFunc {
  return { handler: (n: number) => n + 50 }  // function exported → should NOT fix
}

// Scenario: exported variable with function type → should NOT fix

export let exportedFactorial: (n: number) => number;
exportedFactorial = (n: number): number => { // variable exported → should NOT fix
  if (n <= 1) { return 1; }
  return n * exportedFactorial(n - 1);
};

// Scenario: shared type alias, one function has multiple returns, first return no division but second has → should NOT fix

type MultiReturnSharedType = (n: number) => number

function multiReturnIntUsage(): MultiReturnSharedType {
  return (n: number) => n + 50 // n only used as int
}

function multiReturnMixedUsage(): MultiReturnSharedType {
  if (true) {
    return (n: number) => n + 50 // first return, no division
  }
  return (n: number) => n / 2 // second return, has division → should NOT fix
}

// Scenario: nested arrow function, inner arrow param detected → should fix companion on outer return type

function nestedArrowTest(): void {
  const outerFunc = (outer: number): ((inner: int) => number) => { // should fix return type
    return (inner: int) => inner + 50 // should fix inner param
  }
  const innerFunc = outerFunc(300)
  const result = innerFunc(400)
  const arr: number[] = new Array(result)
}
