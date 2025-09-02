/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
import {
  any_var,
  symbol_var,
  user,
  unknown_var,
  function_var,
  func_type2,
  objectLiteral_var,
  objectLiteral_var2,
  objectLiteral_var3,
  objectLiteral_var32,
  objectLiteral_var4,
  objectLiteral_var5,
  objectLiteral_var51,
  objectLiteral_var6,
  enum_var,
  func_type,
  constructor_type,
  objIndexSignature_var,
  Intersection_obj,
  KeyOf_Type,
  de,
  key,
  keyFuns,
  Keys,
  SomeType,
  Person,
  Person1,
  ElementType,
  NameOrAddress,
  getInfo,
  IndexAccess,
  UserValueTypes,
  stringType,
  NumbersOnly,
  ReturnVal,
  Num,
  typeOf_type,
  typeOf_type1,
  templateLiteralType,
  TemplateLiteralType,
  NameChanged,
  testTemplateLiteralType,
  Test,
  ReadonlyUser,
  Partial,
  OptionalUser,
  Mapped,
  sumType
} from "./ignore_files/unique_types2"

class TestHelper {
  test(arg0: () => boolean, arg1: string) {
    throw new Error("Method not implemented.");
  }
  constructor(name: string) {
  }
}
export function test_unique_type(testCaseRet: Array) {
  let test_helper = new TestHelper("TEST_UNIQUE_TYPE");
  
  test_helper.test(() => {
    return typeof any_var === 'object'   //error
  }, "any_var ")

  test_helper.test(() => {
    return typeof unknown_var === 'number'   //error
  }, "unknown_var ")

  test_helper.test(() => {
    return typeof symbol_var === 'object'  //error
  }, "symbol_var ")

  test_helper.test(() => {
    return function_var() === true   //error
  }, "function_var ")

  test_helper.test(() => {
    return enum_var.a === 0   //error
  }, "enum_var ")

  test_helper.test(() => {
    const res = new objectLiteral_var6().set({});  //error
    const res2 = new objectLiteral_var6().set1(true);  //error
    const res3 = new objectLiteral_var6().set2({},true);  //error
    const res3 = new objectLiteral_var6().set3();
    return typeof res === 'object'
  }, "objectLiteral_var7 ")

  test_helper.test(() => {
    const res = new objectLiteral_var6().get();  //error
    const res2 = new objectLiteral_var6().get1();  //error
    const res3 = new objectLiteral_var6().get2();  //error
    const res4 = new objectLiteral_var6().get3();  //error
    const res5 = new objectLiteral_var6().get4();
    return typeof res === 'object'
  }, "objectLiteral_var6 ")

  test_helper.test(() => {
    typeof objectLiteral_var2 === 'object'   //error
    typeof objectLiteral_var3 === 'object'
    typeof objectLiteral_var32 === 'object'   //error
    typeof objectLiteral_var4 === 'object'   //error
    typeof objectLiteral_var5 === 'object'   //error
    typeof objectLiteral_var51 === 'object'   //error
    return typeof objectLiteral_var === 'object'   //error
  }, "objectLiteral_var ")

  test_helper.test(() => {
    console.log(typeOf_type)
    console.log(typeOf_type1+'')  //error
    return typeof user === 'object'    //error
  }, "typeof")

  test_helper.test(() => {
    let fun: func_type = (arg: number) => {  //error
      return arg.toString();
    };
    let fun2: func_type2 = (arg: number) => {  //error
    };
    return fun2(1) === '111'
  }, "func_type ")

  test_helper.test(() => {
    return typeof user === 'object'   //error
  }, "user ")

  test_helper.test(() => {
    // const aa  = '' as SomeType;
    return typeof ('object' as SomeType) === 'object'   //error
  }, "SomeType ")

  
  test_helper.test(() => {
    return objIndexSignature_var[0] === "zero"   //error
  }, "objIndexSignature_var ")

  test_helper.test(() => {
    return Intersection_obj.a === 10 && Intersection_obj.b === 'hello'   //error*2
  }, "Intersection_obj ")

  test_helper.test(() => {
    let keyof_var1: KeyOf_Type = 'a';   //error
    let kk = de;   //error
    console.log(key)   //error
    keyFuns();   //error
    const keys = new Keys();
    keys.set(undefined);   //error
    new Keys().get();   //error
    keys.get1();   //error
    return keyof_var1 === 'a'
  }, "KeyOf_Type ")

  test_helper.test(() => {
    let a :Person= {name:'a',age:1};   //error
    a=Person1;  //error
    const b = 'a' as ElementType;   //error
    getInfo(b) as NameOrAddress;   //error*2
    typeof new IndexAccess().par;   //error
    const indexAcc = new IndexAccess();
    indexAcc.set('','');   //error
    indexAcc.get('');   //error
    let c = '' as UserValueTypes   //error
    return typeof a === 'object'
  }, "indexed access type")

  test_helper.test(() => {
    new Test().returnStr();  //error?
    return typeof stringType === 'object'   //error?
  }, "stringType ")

  test_helper.test(() => {
    const c:NumbersOnly|undefined = undefined;   //error
    let d:ReturnVal = c as Num;  //error*2
    return typeof c === 'boolean'
  }, "conditional types ")

  test_helper.test(() => {
    const c:ReadonlyUser|OptionalUser|undefined = undefined;   //error*2
    let d:Mapped|boolean = false;   //error
    let e : Partial<TestHelper>;   //error
    return typeof c === 'boolean'
  }, "mapped types ")

  test_helper.test(() => {
    const res: NameChanged = testTemplateLiteralType('');   //error*2
    let b :TemplateLiteralType;   //error
    return typeof templateLiteralType !== "undefined"   //error
  }, "templateLiteralType ")

  test_helper.test(() => {
    let instance = new constructor_type("Alice");   //error
    return instance.name === 'Alice'
  }, "constructor_type ")

  test_helper.test(() => {
    return typeof sumType !== "undefined"   //error
  }, "sumType ")
  
}