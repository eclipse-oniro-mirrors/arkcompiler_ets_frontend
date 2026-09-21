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

// Test for NumericLocalReferenceResolver and NumericParameterFlowAnalyzer branches

// Global variable usage
let globalNumber: number = 10; // arkts-builtin-api-num2int

function testGlobalUsage(): void {
  console.log(globalNumber);
}

// Parameter flow through arrow functions
function testParameterFlow(param: number): void { // arkts-builtin-api-num2int
  const arrowFunc = (x: number) => x + param;
  arrowFunc(5);
}

// Parameter flow through promises
function testPromiseFlow(): void {
  const num: number = 42; // arkts-builtin-api-num2int
  Promise.resolve(100)
    .then((value: number) => {
      console.log(value + num);
    });
}

// Closure field reference
function testClosureReference(): void {
  let outerNum: number = 5; // arkts-builtin-api-num2int
  function innerFunc() {
    console.log(outerNum / 2);
  }
  innerFunc();
}

// Anonymous class field initialization
class OuterClass {
  private outerField: number = 10; // arkts-builtin-api-num2int
  
  createAnonymous(): void {
    const divider: number = 2; // arkts-builtin-api-num2int
    const anonymous = {
      value: this.outerField / divider
    };
    console.log(anonymous.value);
  }
}

// Array derived class usage
class MyArray extends Array<number> {
  constructor() {
    super();
  }
  
  testMethod(): void {
    const idx: number = 0; // arkts-builtin-api-num2int
    console.log(this[idx]);
  }
}

// Multiple parameter usage
function testMultipleParameters(n1: number, n2: number): void { // arkts-builtin-api-num2int
  console.log(n1 + n2);
}

// Parameter in callback chain
function testCallbackChain(val: number): void { // arkts-builtin-api-num2int
  [1, 2, 3].forEach((item: number) => {
    console.log(item + val);
  });
}

// Outer method parameter access
function outerMethodAccess(): void {
  const outerLocal: number = 100; // arkts-builtin-api-num2int
  
  function nestedFunction() {
    function deeplyNested() {
      console.log(outerLocal);
    }
    deeplyNested();
  }
  
  nestedFunction();
}// Test for NumericParameterFlowAnalyzer: Promise.then and closure scenarios

function testPromiseThenWithNumericParam(): void {
  // Promise.then with numeric parameter
  const promise = Promise.resolve(10)
  promise.then((value: number) => {
    const arr: number[] = new Array(value) // arkts-numeric-semantic-array
    return value
  })
  
  // Promise.then chain
  Promise.resolve(5)
    .then((v1: number) => {
      const arr1: number[] = new Array(v1) // arkts-numeric-semantic-array
      return v1 * 2
    })
    .then((v2: number) => {
      const arr2: number[] = new Array(v2) // arkts-numeric-semantic-array
      return v2
    })
}

function testClosureWithNumericParam(): void {
  // Closure capturing numeric parameter
  function outerFunc(outerParam: number): void {
    const closureFunc = () => {
      const arr: number[] = new Array(outerParam) // arkts-numeric-semantic-array
    }
    closureFunc()
  }
  
  // Nested closures
  function nestedOuter(param1: number): () => void {
    return () => {
      const innerClosure = () => {
        const arr: number[] = new Array(param1) // arkts-numeric-semantic-array
      }
      innerClosure()
    }
  }
}

function testArrowFunctionAsCallback(): void {
  // Arrow function passed as callback
  function acceptCallback(cb: (n: number) => void): void {
    cb(10)
  }
  
  acceptCallback((param: number) => {
    const arr: number[] = new Array(param) // arkts-numeric-semantic-array
  })
  
  // Multiple callbacks
  function multiCallback(cb1: (n1: number) => void, cb2: (n2: number) => void): void {
    cb1(5)
    cb2(10)
  }
  
  multiCallback(
    (p1: number) => {
      const arr1: number[] = new Array(p1) // arkts-numeric-semantic-array
    },
    (p2: number) => {
      const arr2: number[] = new Array(p2) // arkts-numeric-semantic-array
    }
  )
}

class ArrowMethodClass {
  testMethodAsCallback(): void {
    function acceptMethod(method: (n: number) => number): void {
      console.log(method(20))
    }
    
    acceptMethod(this.methodWithNumericParam)
  }
  
  methodWithNumericParam(param: number): number {
    const arr: number[] = new Array(param) // arkts-numeric-semantic-array
    return param
  }
}
// Comprehensive test for low coverage files: ParameterFlow, FieldUsage, ExpressionUsage, LocalReference

// ========== NumericParameterFlowAnalyzer Tests ==========

// Test: checkAllArgsOfParameter - parameter flow analysis
function testLowCovParameterFlow(param1: number, param2: number): number {
  // Branch: checkAllArgsOfParameter path
  let local1 = param1 // arkts-numeric-semantic-param-flow
  let local2 = param2 // arkts-numeric-semantic-param-flow
  return local1 + local2 // arkts-numeric-semantic-param-flow
}

// Test: checkClosureFieldRef - closure parameter capture
function testClosureCapture(outerParam: number): () => number {
  // Branch: ClosureFieldRef with LexicalEnvType
  let captured = outerParam // arkts-numeric-semantic-param-flow
  return (): number => captured // arkts-numeric-semantic-param-flow
}

// Test: isFromParameter - parameter reference check
function testFromParameter(numParam: number): void {
  // Branch: rightOp is ArkParameterRef
  let fromParam = numParam // arkts-numeric-semantic-param-flow
  
  // Branch: stmt not ArkAssignStmt
  if (numParam > 0) {
    let arr: number[] = new Array(numParam) // arkts-numeric-semantic-array
  }
}

// Test: checkFromStmt with different paths
function testParamFlowPaths(param: number): number {
  // Branch: normal flow
  let v1 = param // arkts-numeric-semantic-param-flow
  
  // Branch: multiple assignments
  let v2 = v1
  let v3 = v2
  
  // Branch: return value
  return v3 // arkts-numeric-semantic-param-flow
}

// Test: Promise then/call scenarios
function testLowCovPromiseFlow(numParam: number): Promise<number> {
  // Branch: Promise class methods
  return Promise.resolve(numParam) // arkts-numeric-semantic-param-flow
}

async function testAsyncParamFlow(asyncParam: number): Promise<number> {
  // Branch: async function parameter
  let localAsync = asyncParam // arkts-numeric-semantic-param-flow
  return localAsync
}

// ========== NumericFieldUsageAnalyzer Tests ==========

// Test: checkFieldRef - builtin field type
class TestBuiltinField {
  // Branch: builtin field type int/long
  length: number = 10 // arkts-numeric-semantic-ordinary
  
  // Branch: builtin field type number
  size: number = 5 // arkts-numeric-semantic-ordinary
}

// Test: checkFieldRef - SDK field
class TestSdkField {
  // Branch: field from SDK
  sdkField: number = 20 // arkts-numeric-semantic-sdk
}

// Test: checkFieldRef - NamespaceSignature
namespace TestNamespace {
  export const namespaceField: number = 30 // arkts-numeric-semantic-ordinary
}

// Test: checkFieldRef - class derived from Array
class TestArrayDerived extends Array<number> {
  // Branch: length field from array-derived class
  customLength: number = 15 // arkts-numeric-semantic-array
}

// Test: checkFieldRef - ARKTS1_2 vs other language
class TestLanguageCheck {
  // Branch: language check
  langField: number = 25 // arkts-numeric-semantic-ordinary
}

// Test: field in different contexts
function testFieldContext(): void {
  class FieldTestClass {
    static staticField: number = 40 // arkts-numeric-semantic-ordinary
    instanceField: number = 35 // arkts-numeric-semantic-ordinary
  }
  
  // Branch: static field access
  let staticAccess = FieldTestClass.staticField // arkts-numeric-semantic-ordinary
  
  // Branch: instance field access
  let instance = new FieldTestClass()
  let instanceAccess = instance.instanceField // arkts-numeric-semantic-ordinary
  
  // Branch: field in expression
  let exprField = instance.instanceField + 5 // arkts-numeric-semantic-ordinary
}

// Test: field assignment chain
class TestFieldChain {
  field1: number = 10 // arkts-numeric-semantic-ordinary
  field2: number = 20 // arkts-numeric-semantic-ordinary
  
  testChain(): void {
    // Branch: field assignment chain
    let local1 = this.field1 // arkts-numeric-semantic-ordinary
    let local2 = this.field2 // arkts-numeric-semantic-ordinary
    let combined = local1 + local2 // arkts-numeric-semantic-ordinary
  }
}

// ========== NumericExpressionUsageAnalyzer Tests ==========

// Test: expression in various contexts
function testExpressionContexts(): void {
  // Branch: expression in assignment
  let expr1: number = 10 + 5 // arkts-numeric-semantic-ordinary
  
  // Branch: expression in array index
  let arr: number[] = new Array(10 + 5) // arkts-numeric-semantic-array
  
  // Branch: expression in function call
  function acceptExpr(n: number): void {}
  acceptExpr(10 + 5) // arkts-numeric-semantic-ordinary
  
  // Branch: expression in return
  function returnExpr(): number {
    return 10 + 5 // arkts-numeric-semantic-ordinary
  }
  
  // Branch: expression in condition
  if (10 + 5 > 15) {
    let condResult: number = 20 // arkts-numeric-semantic-ordinary
  }
}

// Test: nested expressions
function testNestedExpressions(): void {
  // Branch: nested binary expressions
  let nested1: number = (10 + 5) * 2 // arkts-numeric-semantic-ordinary
  let nested2: number = 10 + (5 * 2) // arkts-numeric-semantic-ordinary
  let nested3: number = ((10 + 5) - 3) / 2 // arkts-numeric-semantic-ordinary
  
  // Branch: expression chain
  let chain1 = 10
  let chain2 = chain1 + 5
  let chain3 = chain2 * 2
  let chain4: number = chain3 // arkts-numeric-semantic-ordinary
}

// Test: expression with different operators
function testExpressionOperators(): void {
  // Branch: arithmetic operators
  let add: number = 10 + 5 // arkts-numeric-semantic-ordinary
  let sub: number = 10 - 5 // arkts-numeric-semantic-ordinary
  let mul: number = 10 * 5 // arkts-numeric-semantic-ordinary
  let div: number = 10 / 5 // arkts-numeric-semantic-ordinary
  let mod: number = 10 % 5 // arkts-numeric-semantic-ordinary
  let pow: number = 10 ** 2 // arkts-numeric-semantic-ordinary
  
  // Branch: comparison in expression context
  let cmpResult = 10 > 5 ? 10 : 5 // should not trigger numeric semantic
}

// ========== NumericLocalReferenceResolver Tests ==========

// Test: local reference resolution
function testLocalReference(): void {
  // Branch: simple local reference
  let local1: number = 10 // arkts-numeric-semantic-ordinary
  let local2 = local1
  let local3 = local2
  
  // Branch: local used in different contexts
  let arr: number[] = new Array(local3) // arkts-numeric-semantic-array
  function acceptLocal(n: number): void {}
  acceptLocal(local3) // arkts-numeric-semantic-ordinary
  
  // Branch: local in return
  function returnLocal(): number {
    let retLocal: number = 15 // arkts-numeric-semantic-ordinary
    return retLocal
  }
}

// Test: local reference with mutations
function testLocalMutation(): void {
  // Branch: local mutation
  let mutLocal: number = 10 // arkts-numeric-semantic-ordinary
  mutLocal = 20 // should still be tracked
  
  // Branch: local with operations
  let opLocal: number = 15 // arkts-numeric-semantic-ordinary
  opLocal = opLocal + 5 // arkts-numeric-semantic-ordinary
}

// Test: local reference across scopes
function testLocalScopes(): void {
  // Branch: local in outer scope
  let outerLocal: number = 25 // arkts-numeric-semantic-ordinary
  
  // Branch: inner function
  function innerFunc(): number {
    // Branch: inner local
    let innerLocal: number = 30 // arkts-numeric-semantic-ordinary
    return innerLocal
  }
  
  // Branch: local captured by closure
  function closureCapture(): () => number {
    let captured: number = 35 // arkts-numeric-semantic-ordinary
    return (): number => captured // arkts-numeric-semantic-ordinary
  }
}

// Test: local reference with fields
class TestLocalFieldInteraction {
  field: number = 40 // arkts-numeric-semantic-ordinary
  
  testInteraction(): void {
    // Branch: local from field
    let fromField = this.field // arkts-numeric-semantic-ordinary
    
    // Branch: field from local
    let localAssign: number = 45 // arkts-numeric-semantic-ordinary
    this.field = localAssign
  }
}

// ========== Combined Complex Scenarios ==========

// Test: parameter to field chain
class TestParamFieldChain {
  field: number = 0
  
  constructor(param: number) {
    // Branch: parameter to field assignment
    this.field = param // arkts-numeric-semantic-param-flow
  }
  
  method(): number {
    // Branch: field back to local
    let local = this.field // arkts-numeric-semantic-ordinary
    return local // arkts-numeric-semantic-ordinary
  }
}

// Test: expression with parameter
function testExprWithParam(param: number): number {
  // Branch: parameter in expression
  let exprParam = param + 5 // arkts-numeric-semantic-param-flow
  
  // Branch: nested expression with parameter
  let nestedParam = (param * 2) + 10 // arkts-numeric-semantic-param-flow
  
  return exprParam + nestedParam // arkts-numeric-semantic-param-flow
}

// Test: field and expression interaction
class TestFieldExprInteraction {
  static staticNum: number = 50 // arkts-numeric-semantic-ordinary
  
  testInteraction(param: number): void {
    // Branch: field and parameter
    let combined = TestFieldExprInteraction.staticNum + param // arkts-numeric-semantic-param-flow
    
    // Branch: multiple field references
    let field1 = TestFieldExprInteraction.staticNum // arkts-numeric-semantic-ordinary
    let field2 = TestFieldExprInteraction.staticNum // arkts-numeric-semantic-ordinary
    let fieldExpr = field1 + field2 // arkts-numeric-semantic-ordinary
  }
}

// Test: async and closure combined
async function testAsyncClosureCombined(asyncParam: number): Promise<number> {
  // Branch: async parameter in closure
  function makeAsyncClosure(): () => number {
    let closureVar = asyncParam // arkts-numeric-semantic-param-flow
    return (): number => closureVar // arkts-numeric-semantic-param-flow
  }
  
  let closureFunc = makeAsyncClosure()
  return closureFunc()
}

// Test: array with parameter and expression
function testArrayParamExpr(param: number): void {
  // Branch: array size from parameter
  let arr1: number[] = new Array(param) // arkts-numeric-semantic-array
  
  // Branch: array size from expression with parameter
  let arr2: number[] = new Array(param + 10) // arkts-numeric-semantic-array
  
  // Branch: array with field and parameter
  class ArrayTest {
    static sizeField: number = 5 // arkts-numeric-semantic-ordinary
  }
  let arr3: number[] = new Array(ArrayTest.sizeField + param) // arkts-numeric-semantic-array
}

// Test: complex flow scenarios
function testComplexFlow(complexParam: number): number {
  // Branch: parameter to multiple locals
  let local1 = complexParam // arkts-numeric-semantic-param-flow
  let local2 = complexParam // arkts-numeric-semantic-param-flow
  
  // Branch: locals to expression
  let expr1 = local1 + local2 // arkts-numeric-semantic-param-flow
  
  // Branch: expression to field
  class ComplexClass {
    field: number = 0
  }
  let complexObj = new ComplexClass()
  complexObj.field = expr1
  
  // Branch: field back to return
  return complexObj.field // arkts-numeric-semantic-param-flow
}
