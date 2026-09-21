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

// Test for NumericParameterFlowAnalyzer: parameter flow through nested calls

function outerFunc1(outerParam: number): void {
  innerFunc1(outerParam)
}

function innerFunc1(innerParam: number): void {
  const arr: number[] = new Array(innerParam) // arkts-numeric-semantic-array
}

function outerFunc2(outerParam: number): number {
  return middleFunc2(outerParam)
}

function middleFunc2(middleParam: number): number {
  return innerFunc2(middleParam)
}

function innerFunc2(innerParam: number): number {
  const arr: number[] = new Array(innerParam) // arkts-numeric-semantic-array
  return innerParam
}

class ParamFlowClass {
  methodA(param: number): void {
    this.methodB(param)
  }
  
  methodB(param: number): void {
    this.methodC(param)
  }
  
  methodC(param: number): void {
    const arr: number[] = new Array(param) // arkts-numeric-semantic-array
  }
  
  recursiveMethod(depth: number, value: number): void {
    if (depth > 0) {
      this.recursiveMethod(depth - 1, value)
    } else {
      const arr: number[] = new Array(value) // arkts-numeric-semantic-array
    }
  }
}

function callbackFlow(param: number): void {
  const callback = (cbParam: number) => {
    const arr: number[] = new Array(cbParam) // arkts-numeric-semantic-array
  }
  callback(param)
}

function conditionalFlow(param1: number, param2: number): void {
  if (param1 > 0) {
    const arr1: number[] = new Array(param1) // arkts-numeric-semantic-array
  } else {
    const arr2: number[] = new Array(param2) // arkts-numeric-semantic-array
  }
}

function loopFlow(count: number): void {
  for (let i = 0; i < count; i++) {
    const arr: number[] = new Array(i) // arkts-numeric-semantic-array
  }
}

function multiParamFlow(a: number, b: number, c: number): void {
  const arr1: number[] = new Array(a) // arkts-numeric-semantic-array
  const arr2: number[] = new Array(b) // arkts-numeric-semantic-array
  const arr3: number[] = new Array(c) // arkts-numeric-semantic-array
}

function arrowFlow(param: number): void {
  const arrowFunc = (p: number) => {
    const nestedArrow = (np: number) => {
      const arr: number[] = new Array(np) // arkts-numeric-semantic-array
    }
    nestedArrow(p)
  }
  arrowFunc(param)
}