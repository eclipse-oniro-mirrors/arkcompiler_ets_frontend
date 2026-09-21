/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

const ORDINARY_VALUES: string[] = ['zero', 'one', 'two', 'three', 'four'];
const GLOBAL_INDEX = 3; // arkts-numeric-semantic

function localAnnotatedIndex(): void {
  let localIndex: number = 1; // arkts-numeric-semantic
  console.log(ORDINARY_VALUES[localIndex]);
}

function copiedIndex(): void {
  const sourceIndex = 2; // arkts-numeric-semantic
  const copiedIndex = sourceIndex; // arkts-numeric-semantic
  console.log(ORDINARY_VALUES[copiedIndex]);
}

function expressionAndUnaryIndex(): void {
  let offsetIndex = 1 + 2; // arkts-numeric-semantic
  let backIndex = -1; // arkts-numeric-semantic
  console.log(ORDINARY_VALUES[offsetIndex + backIndex]);
  console.log(ORDINARY_VALUES[~backIndex]);
}

function forLoopIndex(): void {
  for (let index = 0; index < ORDINARY_VALUES.length; index++) { // arkts-numeric-semantic
    console.log(ORDINARY_VALUES[index]);
  }
}

function globalIndexUse(): void {
  console.log(ORDINARY_VALUES[GLOBAL_INDEX]);
}

function capturedIndexUse(): void {
  let capturedIndex = 2; // arkts-numeric-semantic
  let readValue = (): string => {
    return ORDINARY_VALUES[capturedIndex];
  };
  console.log(readValue());
}

function postIncrementIndex(): void {
  let index = 1; // arkts-numeric-semantic
  console.log(ORDINARY_VALUES[index++]);
}

function integerLiteralUsedAsNumber(): void {
  let dividend = 1; // arkts-numeric-semantic
  let divisor = 2; // arkts-numeric-semantic
  let quotient = dividend / divisor; // arkts-numeric-semantic
  console.log(quotient);
}

function mixedIntAndNumberUse(): void {
  let index = 2; // arkts-numeric-semantic
  console.log(ORDINARY_VALUES[index]); // arkts-numeric-semantic
  let ratio = index / 2.0; // arkts-numeric-semantic
  console.log(ratio);
}

let dynamicNumberProvider: Function = (value: number): number => {
  return value;
};

class NumericComponentLike {
  borderWidth(value: number): void {
    console.log(value);
  }
}

function divisionUsedAsNestedDynamicFunctionArg(): void {
  let component = new NumericComponentLike();
  component.borderWidth(dynamicNumberProvider(2 / 3)); // arkts-numeric-semantic
}

class OrdinaryIndexHolder {
  private instanceIndex = 1; // arkts-numeric-semantic
  private static staticIndex = 2; // arkts-numeric-semantic

  readInstance(): string {
    return ORDINARY_VALUES[this.instanceIndex];
  }

  static readStatic(): string {
    return ORDINARY_VALUES[OrdinaryIndexHolder.staticIndex];
  }
}

function variableArithmeticTest(): void {
  let a: number = 10; // arkts-numeric-semantic
  let b: number = 5; // arkts-numeric-semantic
  let sum = a + b;
  let arr: number[] = new Array(sum); // arkts-numeric-semantic
  console.log(arr.length);
}

function complexArithmeticTest(): void {
  let x: number = 20; // arkts-numeric-semantic
  let y: number = 10; // arkts-numeric-semantic
  let z: number = 5; // arkts-numeric-semantic
  let result = (x + y) * z - 50;
  let arr: number[] = new Array(result); // arkts-numeric-semantic
  console.log(arr.length);
}

function shiftOperationTest(): void {
  let value: number = 16; // arkts-numeric-semantic
  let shifted = value << 2;
  let arr: number[] = new Array(shifted); // arkts-numeric-semantic
  console.log(arr.length);
}

function comparisonResultTest(): void {
  let a: number = 10; // arkts-numeric-semantic
  let b: number = 20; // arkts-numeric-semantic
  let max = a > b ? a : b;
  let arr: number[] = new Array(max); // arkts-numeric-semantic
  console.log(arr.length);
}

function functionReturnTest(): void {
  function getSize(): number {
    return 12; // arkts-numeric-semantic
  }
  let arr: number[] = new Array(getSize()); // arkts-numeric-semantic
  console.log(arr.length);
}

function methodCallTest(): void {
  class Calculator {
    getValue(): number {
      return 8; // arkts-numeric-semantic
    }
  }
  let calc = new Calculator();
  let arr: number[] = new Array(calc.getValue()); // arkts-numeric-semantic
  console.log(arr.length);
}
// Test for NumericSignatureMatcher: Interface type scenarios

interface NumericInterface {
  value: number // arkts-numeric-semantic - interface field
  method(param: number): number // arkts-numeric-semantic - interface method
}

class InterfaceImpl implements NumericInterface {
  value: number = 100 // arkts-numeric-semantic - implementing interface field
  
  method(param: number): number { // arkts-numeric-semantic - implementing interface method
    return this.value + param
  }
}

function interfaceTest(): void {
  const instance: NumericInterface = new InterfaceImpl()
  const result = instance.method(200)
  const arr: number[] = new Array(result)
}

interface NumericPropertyInterface {
  readonly property: number // arkts-numeric-semantic - readonly interface property
}

function readonlyPropertyTest(): void {
  const obj: NumericPropertyInterface = { property: 300 }
  const arr: number[] = new Array(obj.property)
}

interface NumericOptionalInterface {
  required: number // arkts-numeric-semantic - required interface field
  optional?: number // arkts-numeric-semantic - optional interface field
}

function optionalInterfaceTest(): void {
  const obj: NumericOptionalInterface = { required: 400, optional: 500 }
  const arr: number[] = new Array(obj.required + (obj.optional ?? 0))
}

interface NumericMethodInterface {
  (n: number): number // arkts-numeric-semantic - callable interface
}

function callableInterfaceTest(callable: NumericMethodInterface): void {
  const result = callable(600)
  const arr: number[] = new Array(result)
}

interface NumericIndexInterface {
  [key: string]: number // arkts-numeric-semantic - index signature interface
}

function indexInterfaceTest(): void {
  const obj: NumericIndexInterface = {
    first: 700,
    second: 800
  }
  const arr: number[] = new Array(obj['first'])
}

interface NumericExtendsInterface extends NumericInterface {
  extra: number // arkts-numeric-semantic - extending interface
}

function extendsInterfaceTest(): void {
  interface ExtendedImpl extends NumericInterface {
    extra: number
  }
  
  const obj: ExtendedImpl = {
    value: 900,
    extra: 1000,
    method: (param: number) => param + 50
  }
  
  const arr: number[] = new Array(obj.extra)
}

interface NumericMultipleInterface {
  primary: number // arkts-numeric-semantic
  secondary: number // arkts-numeric-semantic
}

function multipleFieldInterfaceTest(): void {
  const obj: NumericMultipleInterface = {
    primary: 1100,
    secondary: 1200
  }
  
  const arr1: number[] = new Array(obj.primary)
  const arr2: number[] = new Array(obj.secondary)
}
// Test for NumericFieldUsageAnalyzer branches: private, readonly, enum, interface, array length, namespace

// Private field usage
class PrivateFieldClass {
  private privateField: number = 10 // arkts-numeric-semantic
  
  usePrivateField(): void {
    const arr1: number[] = new Array(this.privateField) // arkts-numeric-semantic-array
  }
  
  modifyPrivateField(): void {
    this.privateField = 15 // Should not trigger warning on modification
    const arr2: number[] = new Array(this.privateField) // arkts-numeric-semantic-array
  }
}

// Readonly field usage
class ReadonlyFieldClass {
  private readonly readonlyField: number = 20 // arkts-numeric-semantic
  
  constructor() {
    // Constructor initialization - should check
    const arr3: number[] = new Array(this.readonlyField) // arkts-numeric-semantic-array
  }
  
  useReadonlyField(): void {
    const arr4: number[] = new Array(this.readonlyField) // arkts-numeric-semantic-array
  }
}

// Readonly field with initialization from parameter
class ReadonlyInitClass {
  private readonly initField: number // arkts-numeric-semantic
  
  constructor(initValue: number) {
    this.initField = initValue
    const arr5: number[] = new Array(this.initField) // arkts-numeric-semantic-array
  }
}

// Field with getter/setter
class GetterSetterClass {
  private _fieldWithGetterSetter: number = 25 // Should not trigger - has getter/setter
  
  get fieldWithGetterSetter(): number {
    return this._fieldWithGetterSetter
  }
  
  set fieldWithGetterSetter(value: number) {
    this._fieldWithGetterSetter = value
  }
  
  useFieldWithAccessor(): void {
    const arr6: number[] = new Array(this.fieldWithGetterSetter) // Should not trigger - accessor
  }
}

// Enum with numeric field
enum NumericEnum {
  First = 1,
  Second = 2,
  Third = 3
}

function testEnumField(): void {
  const enumValue: NumericEnum = NumericEnum.Second
  const arr7: number[] = new Array(enumValue) // Should not trigger - enum value
}

// Interface with numeric property
interface FieldNumericInterface {
  value: number
}

class InterfaceImplClass implements FieldNumericInterface {
  value: number = 30 // arkts-numeric-semantic
  
  useInterfaceField(): void {
    const arr8: number[] = new Array(this.value) // arkts-numeric-semantic-array
  }
}

// Array-derived class length field
class ArrayDerivedClass extends Array<number> {
  constructor(size: number) {
    super(size)
  }
  
  useLengthField(): void {
    const len: number = this.length // Should not trigger - array length is int
    const arr9: number[] = new Array(len) // Should not trigger
  }
}

// Namespace field
namespace NumericNamespace {
  const namespaceField: number = 35 // arkts-numeric-semantic
  
  export function useNamespaceField(): void {
    const arr10: number[] = new Array(namespaceField) // arkts-numeric-semantic-array
  }
}

NumericNamespace.useNamespaceField()

// Cross-class field reference
class FieldOwnerClass {
  public publicField: number = 40 // arkts-numeric-semantic
}

class FieldUserClass {
  useOtherClassField(owner: FieldOwnerClass): void {
    const arr11: number[] = new Array(owner.publicField) // Should not trigger - cross-class
  }
}

// Static field
class StaticFieldClass {
  private static staticField: number = 45 // arkts-numeric-semantic
  
  static useStaticField(): void {
    const arr12: number[] = new Array(StaticFieldClass.staticField) // arkts-numeric-semantic-array
  }
}

StaticFieldClass.useStaticField()

// Protected field
class ProtectedFieldClass {
  protected protectedField: number = 50 // arkts-numeric-semantic
  
  useProtectedField(): void {
    const arr13: number[] = new Array(this.protectedField) // arkts-numeric-semantic-array
  }
}

class DerivedProtectedClass extends ProtectedFieldClass {
  useInheritedProtectedField(): void {
    const arr14: number[] = new Array(this.protectedField) // arkts-numeric-semantic-array
  }
}

// Field in anonymous class
function testAnonymousClassFieldUsage(): void {
  const localForAnonymous2: number = 55 // arkts-numeric-semantic
  
  const anonymousObj2 = {
    anonField: localForAnonymous2,
    getAnonField(): number {
      return this.anonField
    }
  }
  
  const arr15: number[] = new Array(anonymousObj2.anonField) // Should not trigger - anonymous class field
}

// Multiple fields with same name in different classes
class SameNameClass1 {
  sameNameField: number = 60 // arkts-numeric-semantic
}

class SameNameClass2 {
  sameNameField: number = 65 // arkts-numeric-semantic
  
  testSameNameField(): void {
    const arr16: number[] = new Array(this.sameNameField) // arkts-numeric-semantic-array
  }
}

// Field used in nested class
class OuterFieldClass {
  outerField: number = 70 // arkts-numeric-semantic
  
  createNestedClass(): void {
    class NestedClass {
      nestedMethod(outer: OuterFieldClass): void {
        const arr17: number[] = new Array(outer.outerField) // Should not trigger - cross-class
      }
    }
    
    const nested = new NestedClass()
    nested.nestedMethod(this)
  }
}
// Test for NumericFieldUsageAnalyzer: field usage in array operations

class FieldUsageTestClass {
  private sizeField: number = 10
  protected countField: number = 5
  public lengthField: number = 20
  
  testFieldUsage(): void {
    // Direct field usage
    const arr1: number[] = new Array(this.sizeField) // arkts-numeric-semantic-array
    const arr2: number[] = new Array(this.countField) // arkts-numeric-semantic-array
    const arr3: number[] = new Array(this.lengthField) // arkts-numeric-semantic-array
    
    // Field in expression
    const arr4: number[] = new Array(this.sizeField + this.countField) // arkts-numeric-semantic-array
    const arr5: number[] = new Array(this.sizeField * 2) // arkts-numeric-semantic-array
    
    // Field in nested method
    this.useFieldInNested(this.sizeField)
  }
  
  useFieldInNested(size: number): void {
    const arr: number[] = new Array(size) // arkts-numeric-semantic-array
  }
  
  testFieldInLoop(): void {
    for (let i = 0; i < this.countField; i++) {
      const arr: number[] = new Array(this.sizeField) // arkts-numeric-semantic-array
    }
  }
  
  testFieldInConditional(): void {
    if (this.sizeField > 5) {
      const arr: number[] = new Array(this.sizeField) // arkts-numeric-semantic-array
    }
  }
  
  testFieldModified(): void {
    this.sizeField = 15
    const arr: number[] = new Array(this.sizeField) // arkts-numeric-semantic-array
  }
}

class DerivedFieldClass extends FieldUsageTestClass {
  private derivedField: number = 8
  
  testDerivedField(): void {
    const arr1: number[] = new Array(this.derivedField) // arkts-numeric-semantic-array
    const arr2: number[] = new Array(this.countField) // arkts-numeric-semantic-array
    const arr3: number[] = new Array(this.lengthField) // arkts-numeric-semantic-array
  }
}

class NumFieldStaticFieldClass {
  static staticSize: number = 100
  
  testStaticField(): void {
    const arr: number[] = new Array(NumFieldStaticFieldClass.staticSize) // arkts-numeric-semantic-array
  }
}

class NumFieldReadonlyFieldClass {
  readonly readonlyField: number = 25
  
  testReadonlyField(): void {
    const arr: number[] = new Array(this.readonlyField) // arkts-numeric-semantic-array
  }
}

class OptionalFieldClass {
  optionalField?: number
  
  testOptionalField(): void {
    if (this.optionalField !== undefined) {
      const arr: number[] = new Array(this.optionalField) // arkts-numeric-semantic-array
    }
  }
}
// Test for NumericTypeClassifier: int-like types (byte, short, int)

function byteTypeTest(): void {
  const byteValue: number = 127 // arkts-numeric-semantic - byte range
  const arr: number[] = new Array(byteValue)
}

function shortTypeTest(): void {
  const shortValue: number = 32767 // arkts-numeric-semantic - short range
  const arr: number[] = new Array(shortValue)
}

function intTypeTest(): void {
  const intValue: number = 2147483647 // arkts-numeric-semantic - int max
  const arr: number[] = new Array(intValue)
}

function intNegativeTest(): void {
  const negativeInt: number = -100 // arkts-numeric-semantic - negative int
  const arr: number[] = new Array(Math.abs(negativeInt))
}

function zeroIntTest(): void {
  const zero: number = 0 // arkts-numeric-semantic - zero
  const arr: number[] = new Array(zero + 1)
}
// Test for NumericTypeClassifier: long type classification

function longTypeTest(): void {
  const longValue: number = 9223372036854775807 // arkts-numeric-semantic - large number potentially long
  const arr: number[] = new Array(longValue)
}

function longInExpression(): void {
  const bigNum: number = 2147483648 // arkts-numeric-semantic - exceeds int max
  const result = bigNum * 2
  const arr: number[] = new Array(result)
}

function longParameter(param: number): void { // arkts-numeric-semantic - param potentially long
  const arr: number[] = new Array(param)
}

function longReturnType(): number { // arkts-numeric-semantic - return potentially long
  return 9223372036854775800
}

class LongFieldClass {
  private longField: number = 2147483649 // arkts-numeric-semantic - field potentially long
  
  useLongField(): void {
    const arr: number[] = new Array(this.longField)
  }
}
// Test for NumericExpressionUsageAnalyzer: expression usage in array operations

function testExpressionInArray(): void {
  // Arithmetic expression
  const a: number = 10
  const b: number = 5
  const arr1: number[] = new Array(a + b) // arkts-numeric-semantic-array
  const arr2: number[] = new Array(a - b) // arkts-numeric-semantic-array
  const arr3: number[] = new Array(a * b) // arkts-numeric-semantic-array
  const arr4: number[] = new Array(a / b) // arkts-numeric-semantic-array
  const arr5: number[] = new Array(a % b) // arkts-numeric-semantic-array
  const arr6: number[] = new Array(a ** b) // arkts-numeric-semantic-array
  
  // Complex expression
  const c: number = 3
  const d: number = 1
  const arr7: number[] = new Array((a + b) * 2 - 3) // arkts-numeric-semantic-array
  const arr8: number[] = new Array(a + b * c - d) // arkts-numeric-semantic-array
  
  // Expression in nested call
  function getSize(size: number): number {
    return size
  }
  const arr9: number[] = new Array(getSize(a + b)) // arkts-numeric-semantic-array
  
  // Expression with method call
  class MathHelper {
    getValue(): number {
      return 20
    }
  }
  const helper = new MathHelper()
  const arr10: number[] = new Array(helper.getValue() + a) // arkts-numeric-semantic-array
  
  // Expression with ternary operator
  const arr11: number[] = new Array(a > b ? a : b) // arkts-numeric-semantic-array
  
  // Expression with bitwise operators
  const arr12: number[] = new Array(a | b) // arkts-numeric-semantic-array
  const arr13: number[] = new Array(a & b) // arkts-numeric-semantic-array
  const arr14: number[] = new Array(a ^ b) // arkts-numeric-semantic-array
  const arr15: number[] = new Array(a << 2) // arkts-numeric-semantic-array
  const arr16: number[] = new Array(a >> 2) // arkts-numeric-semantic-array
  
  // Expression with unary operators
  const arr17: number[] = new Array(-a) // arkts-numeric-semantic-array
  const arr18: number[] = new Array(+b) // arkts-numeric-semantic-array
  const arr19: number[] = new Array(~a) // arkts-numeric-semantic-array
  
  // Expression in loop
  for (let i = 0; i < 5; i++) {
    const arr: number[] = new Array(i * 2) // arkts-numeric-semantic-array
  }
  
  // Expression in conditional
  if (a > 5) {
    const arr: number[] = new Array(a * 2) // arkts-numeric-semantic-array
  }
  
  // Expression in array literal
  const sizes: number[] = [5, 10, 15]
  const arr20: number[] = new Array(sizes[0] + sizes[1]) // arkts-numeric-semantic-array
  
// Expression with typeof
  const cType: number = 10
  const arr21: number[] = new Array(typeof cType === 'number' ? cType : 0) // invalid for test
}
// 针对 NumericSourceUsageChecker 中未覆盖分支的完整测试

// ========== checkFieldInitializerWithIntLiteral 分支测试 ==========

// Test: 静态初始化方法中的字段赋值 (line 79-84)
class StaticFieldInit {
  static staticField1: number = 100 // arkts-numeric-semantic-ordinary
  static staticField2: number = 200 // arkts-numeric-semantic-ordinary
}

// Test: 实例初始化方法中的字段赋值 (line 79-84)
class InstanceFieldInit {
  instanceField1: number = 150 // arkts-numeric-semantic-ordinary
  instanceField2: number = 250 // arkts-numeric-semantic-ordinary
}

// Test: 字段初始化涉及除法运算 (line 95-98)
class DivisionFieldInit {
  field1: number = 0 // arkts-numeric-semantic-ordinary
  field2: number = 0 // arkts-numeric-semantic-ordinary
  
  initFields(): void {
    this.field1 = 10 / 2 // arkts-numeric-semantic-ordinary
    this.field2 = 20 / 4 // arkts-numeric-semantic-ordinary
  }
}

// Test: 字段初始化使用NumberConstant (line 106-113)
class NumberConstantFieldInit {
  field1: number = 1000 // arkts-numeric-semantic-ordinary
  field2: number = 2000 // arkts-numeric-semantic-ordinary
}

// ========== checkAsyncReturnStmts 分支测试 ==========

// Test: 直接return整型字面量 (line 171-173)
async function asyncReturnIntLiteral(): Promise<number> {
  return 42 // arkts-numeric-semantic-ordinary
}

// Test: return变量 (line 174-181)
async function asyncReturnLocal(): Promise<number> {
  const localVar: number = 50 // arkts-numeric-semantic-ordinary
  return localVar
}

// Test: 多个async return语句 (line 162-166)
async function asyncMultipleReturns(cond: boolean): Promise<number> {
  if (cond) {
    return 100 // arkts-numeric-semantic-ordinary
  }
  return 200 // arkts-numeric-semantic-ordinary
}

// ========== checkDivisionStmtWithNumericLiteral 分支测试 ==========

// Test: leftOp不是number类型但用作函数参数 (line 203-206)
function acceptNumberParam(n: number): void {}

function testDivisionNonNumberType(): void {
  // leftOp不是nearlyNumberType,但是被用作函数参数
  const result = 10 / 2
  acceptNumberParam(result) // arkts-numeric-semantic-ordinary
}

// Test: leftOp是number类型 (line 210-213)
function testDivisionNumberType(): void {
  const result: number = 10 / 2 // arkts-numeric-semantic-ordinary
}

// ========== checkAssignStmtWithNumericLiteral 分支测试 ==========

// Test: leftOp是AbstractFieldRef (line 224-227)
class TestLeftOpFieldRef {
  field: number = 0
  
  testFieldRefAssign(): void {
    this.field = 200 // arkts-numeric-semantic-ordinary
  }
}

// Test: NumberConstant超过INT32边界 (line 242-244)
function testNumberConstantOverflow(): void {
  // INT32边界是2147483647
  const bigNum: number = 2147483648 // arkts-numeric-semantic-ordinary
  const biggerNum: number = 3000000000 // arkts-numeric-semantic-ordinary
}

// ========== checkInvokeExprArgsWithDivision 分支测试 ==========

// Test: 函数参数包含除法运算 (line 262-267)
function acceptDivisionResult(n: number): void {}

function testInvokeArgWithDivision(): void {
  acceptDivisionResult(10 / 2) // arkts-numeric-semantic-ordinary
}

// Test: 多个参数中有除法运算
function multiParamFunc(a: number, b: number, c: number): void {}

function testMultiParamWithDivision(): void {
  multiParamFunc(10 / 2, 20, 30 / 3) // arkts-numeric-semantic-ordinary
}

// ========== checkAbstractExprWithIntLiteral 分支测试 ==========

// Test: ArkUnopExpr - Neg操作符 (line 381-387)
function testArkUnopNeg(): void {
  const positive: number = 100 // arkts-numeric-semantic-ordinary
  const negative: number = -positive // arkts-numeric-semantic-ordinary
}

// Test: ArkUnopExpr - 其他操作符 (line 381-388)
function testArkUnopOther(): void {
  const value: number = 42
  // 位非操作符
  const notValue: number = ~value // arkts-numeric-semantic-ordinary
}

// Test: ArkNormalBinopExpr (line 389-392)
function testArkNormalBinop(): void {
  const left: number = 100 // arkts-numeric-semantic-ordinary
  const right: number = 200 // arkts-numeric-semantic-ordinary
  const result: number = left + right // arkts-numeric-semantic-ordinary
}

// ========== checkBinaryExprWithIntLiteral 分支测试 ==========

// Test: LogicalAnd操作符 (line 405-408)
function testLogicalAnd(): void {
  const left: number = 100 // arkts-numeric-semantic-ordinary
  const right: number = 200 // arkts-numeric-semantic-ordinary
  const result: number = left && right // arkts-numeric-semantic-ordinary
}

// Test: LogicalOr操作符 (line 405-408)
function testLogicalOr(): void {
  const left: number = 100 // arkts-numeric-semantic-ordinary
  const right: number = 200 // arkts-numeric-semantic-ordinary
  const result: number = left || right // arkts-numeric-semantic-ordinary
}

// Test: Division操作符 (line 410-416)
function testBinaryDivision(): void {
  const left: number = 100 // arkts-numeric-semantic-ordinary
  const right: number = 2
  const result: number = left / right // arkts-numeric-semantic-ordinary
}

// Test: Addition操作符 (line 418-426)
function testBinaryAddition(): void {
  const left: number = 100 // arkts-numeric-semantic-ordinary
  const right: number = 200 // arkts-numeric-semantic-ordinary
  const result: number = left + right // arkts-numeric-semantic-ordinary
}

// Test: Subtraction操作符 (line 418-426)
function testBinarySubtraction(): void {
  const left: number = 200 // arkts-numeric-semantic-ordinary
  const right: number = 100 // arkts-numeric-semantic-ordinary
  const result: number = left - right // arkts-numeric-semantic-ordinary
}

// Test: Multiplication操作符 (line 418-426)
function testBinaryMultiplication(): void {
  const left: number = 100 // arkts-numeric-semantic-ordinary
  const right: number = 2
  const result: number = left * right // arkts-numeric-semantic-ordinary
}

// Test: Exponentiation操作符 (line 418-426)
function testBinaryExponentiation(): void {
  const base: number = 2
  const exponent: number = 10
  const result: number = base ** exponent // arkts-numeric-semantic-ordinary
}

// Test: NullishCoalescing操作符 (line 418-426)
function testNullishCoalescing(): void {
  const left: number | null = null
  const right: number = 100 // arkts-numeric-semantic-ordinary
  const result: number = left ?? right // arkts-numeric-semantic-ordinary
}

// Test: BitwiseAnd操作符 (line 428-438)
function testBitwiseAnd(): void {
  const left: number = 100 // arkts-numeric-semantic-ordinary
  const right: number = 200 // arkts-numeric-semantic-ordinary
  const result: number = left & right // arkts-numeric-semantic-ordinary
}

// Test: BitwiseOr操作符 (line 428-438)
function testBitwiseOr(): void {
  const left: number = 100 // arkts-numeric-semantic-ordinary
  const right: number = 200 // arkts-numeric-semantic-ordinary
  const result: number = left | right // arkts-numeric-semantic-ordinary
}

// Test: BitwiseXor操作符 (line 428-438)
function testBitwiseXor(): void {
  const left: number = 100 // arkts-numeric-semantic-ordinary
  const right: number = 200 // arkts-numeric-semantic-ordinary
  const result: number = left ^ right // arkts-numeric-semantic-ordinary
}

// Test: LeftShift操作符 (line 428-438)
function testLeftShift(): void {
  const value: number = 1
  const shift: number = 8
  const result: number = value << shift // arkts-numeric-semantic-ordinary
}

// Test: RightShift操作符 (line 428-438)
function testRightShift(): void {
  const value: number = 256
  const shift: number = 4
  const result: number = value >> shift // arkts-numeric-semantic-ordinary
}

// Test: UnsignedRightShift操作符 (line 428-438)
function testUnsignedRightShift(): void {
  const value: number = 256
  const shift: number = 4
  const result: number = value >>> shift // arkts-numeric-semantic-ordinary
}

// Test: Remainder操作符 (line 428-438)
function testRemainder(): void {
  const left: number = 100 // arkts-numeric-semantic-ordinary
  const right: number = 7
  const result: number = left % right // arkts-numeric-semantic-ordinary
}

// ========== checkValueSensitiveBinaryExpr 分支测试 ==========

// Test: op1和op2都是OnlyUsedAsIntLong (line 452-458)
function testBothOpsIntLong(): void {
  const op1: number = 100 // arkts-numeric-semantic-ordinary
  const op2: number = 200 // arkts-numeric-semantic-ordinary
  const result: number = op1 + op2 // arkts-numeric-semantic-ordinary
}

// Test: op1和op2不都是OnlyUsedAsIntLong (line 460-468)
function testNotBothOpsIntLong(): void {
  const op1: number = 100 // arkts-numeric-semantic-ordinary
  const op2: number = 200.5 // 浮点数
  const result: number = op1 + op2 // arkts-numeric-semantic-ordinary
}

// ========== 综合场景测试 ==========

// Test: 多层嵌套的二元表达式
function testNestedBinaryExpr(): void {
  const a: number = 10 // arkts-numeric-semantic-ordinary
  const b: number = 20 // arkts-numeric-semantic-ordinary
  const c: number = 30 // arkts-numeric-semantic-ordinary
  const result: number = (a + b) * c - a / b // arkts-numeric-semantic-ordinary
}

// Test: 字段初始化和表达式的组合
class ComplexFieldInit {
  field1: number = 0 // arkts-numeric-semantic-ordinary
  field2: number = 0 // arkts-numeric-semantic-ordinary
  
  initComplexFields(): void {
    this.field1 = 10 / 2 // arkts-numeric-semantic-ordinary
    this.field2 = 20 * 3 // arkts-numeric-semantic-ordinary
  }
}

// Test: async函数和除法的组合
async function testAsyncDivision(): Promise<number> {
  const result: number = 100 / 3 // arkts-numeric-semantic-ordinary
  return result
}

// Test: 字段引用和除法的组合
class FieldDivisionTest {
  field: number = 0
  
  testMethod(): void {
    this.field = 50 / 2 // arkts-numeric-semantic-ordinary
  }
}

// Test: 多种操作符的组合
function testMultipleOperators(): void {
  const a: number = 100 // arkts-numeric-semantic-ordinary
  const b: number = 50
  const c: number = 2
  
  const add: number = a + b // arkts-numeric-semantic-ordinary
  const sub: number = a - b // arkts-numeric-semantic-ordinary
  const mul: number = a * c // arkts-numeric-semantic-ordinary
  const div: number = a / c // arkts-numeric-semantic-ordinary
  const mod: number = a % b // arkts-numeric-semantic-ordinary
  const pow: number = c ** 10 // arkts-numeric-semantic-ordinary
}
