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
 unknown_var,
 symbol_var,
 function_var,
 enum_var,
 A,
 B,
 TestHelper,
 throw_number,
 throw_string,
 throw_boolean,
 throw_bigint,
 throw_obj,
 throw_error,
 throwErrorSubClass,
 SubError,
 throwRangeError,
 ObjectLiteralClass,
 ObjectLiteralInter,
 MyClassDecorator,
 propertyDecorator,
 methodDecorator,
 parameterDecorator,
 accessorDecorator,
 readonly1,
 num_boxed,
 bool_boxed,
 str_boxed,
 bigint_boxed,
 ts_object_method,
 ts_object_method_getOwnPropertyDescriptor,
 ts_object_method_getOwnPropertyDescriptors,
 ts_object_method_getOwnPropertyNames,
 ts_object_method_hasOwn,
 ts_object_method_isExtensible,
 ts_object_method_isFrozen,
 ts_object_method_isSealed,
 ts_object_method_keys,
 ts_object_method_values,
 interObj,
 ts_reflect_method,
 ts_reflect_method_apply,
 ts_reflect_method_defineProperty,
 ts_reflect_method_deleteProperty,
 ts_reflect_method_getOwnPropertyDescriptor,
 ts_reflect_method_ownKeys,
 ts_reflect_method_isExtensible,
 ts_reflect_method_set,
 ts_reflect_method_setPrototypeOf
} from "./ignore_files/unique_types";


typeof any_var === 'object';
typeof unknown_var === 'number';
typeof symbol_var === 'object';
function_var.unsafeCall() === true;
A.instance;
let obj: B = { name: "hello" };
enum_var.a === 0;

export function test_ts_non_standard_exception(testCaseRet: Array) {
  let test_helper = new TestHelper("TEST_TS_NON_STANDARD_EXCEPTION");

  test_helper.test(() => {
    try {
      throw_number(); // arkts-interop-ts2s-ts-exception
    } catch (e) {
      return e as number === 123;
    }
    return false;
  }, "e as number === 123");

  test_helper.test(() => {
    try {
      throw_boolean(); // arkts-interop-ts2s-ts-exception
    } catch (e) {
      return e as boolean === true;
    }
    return false;
  }, "e as boolean === true");

  test_helper.test(() => {
    try {
      throw_bigint(); // arkts-interop-ts2s-ts-exception
    } catch (e) {
      return e as bigint === 111n;
    }
    return false;
  }, "e as bigint === 111n");

  test_helper.test(() => {
    try {
      throw_string(); // arkts-interop-ts2s-ts-exception
    } catch (e) {
      return e as string === "error";
    }
    return false;
  }, "e as string === "error"");

  test_helper.test(() => {
    try {
      throw_obj(); // arkts-interop-ts2s-ts-exception
    } catch (e) {
      return e.name === "error";
    }
    return false;
  }, "e.name === "error"");

  test_helper.test(() => {
    try {
      throw_error();
    } catch (e) {
      let errObj: Error = e as Error;
      return errObj.name === "error";
    }
    return false;
  }, "errObj.name === "error"");

  test_helper.test(() => {
    try {
      throwErrorSubClass(); // arkts-interop-ts2s-ts-exception
    } catch (e) {
      let errObj = e as SubError;
      return errObj.extraField === 123 && errObj.foo() === 456;
    }
    return false;
  }, "errObj.extraField === 123 && errObj.foo() === 456");

  test_helper.test(() => {
    let flag = false
    try {
      throwRangeError(); // arkts-interop-ts2s-ts-exception
    } catch (e) {
      let errObj: RangeError = e as RangeError;
      return errObj instanceof RangeError;
    }
    return flag;
  }, "Throw: throwRangeError");

  testCaseRet.push(test_helper.done());
}

export function test_object_literal(testCaseRet: Array) {
  let test_helper = new TestHelper("TEST_OBJECT_LITERAL");

  test_helper.test(() => {
    let obj: ObjectLiteralClass = { name: "hello" }; // arkts-obj-literal-generate-class-instance
    return obj.name === "hello" && obj instanceof ObjectLiteralClass;
  }, "obj.name === "hello"");

  test_helper.test(() => {
    let obj: ObjectLiteralInter = { name: "hello" }; // arkts-obj-literal-generate-class-instance
    return obj.name === "hello" && obj instanceof ObjectLiteralInter;
  }, "obj.name === "hello"");

  testCaseRet.push(test_helper.done());
}

// 1 ArkTS使用TS装饰器
// 规则名 arkts-interop-ts2s-no-ts-decorator
// case 1 类装饰器 top level
@MyClassDecorator // arkts-no-ts-decorators + arkts-interop-ts2s-no-ts-decorator
class K {}

//case 2 类属性装饰器
class MyClass {
  @propertyDecorator// arkts-no-ts-decorators
  myProperty: string;
}

//case 3 方法装饰器
class MathFunctions {
  @methodDecorator// arkts-no-ts-decorators
  double1(value: number): number {
    return value;
  }
}

//case 4 方法装饰器
class MyClass1 {
  log(value: string, @parameterDecorator metadata: any) {// arkts-no-ts-decorators
    console.log(Logged: ${value});
  }
}

//case 5 访问器装饰器
class Person {
  private _name: string;
  @accessorDecorator// arkts-no-ts-decorators
  get name(): string {
    return this._name;
  }
  set name(value: string) {
    this._name = value;
  }
}

//case 6 访问器工厂
class Person1 {
  private _age: number;
  @readonly(true)// arkts-no-ts-decorators
  get age(): number {
    return this._age;
  }
  set age(value: number) {
    this._age = value;
  }
}

namespace NS{
  // case 7 类装饰器 top level
  @MyClassDecorator// arkts-no-ts-decorators + arkts-interop-ts2s-no-ts-decorator
  class K {}

  //case 8 类属性装饰器
  class MyClass {
    @propertyDecorator// arkts-no-ts-decorators
    myProperty: string;
  }

  //case 9 方法装饰器
  class MathFunctions {
    @methodDecorator// arkts-no-ts-decorators
    double1(value: number): number {
      return value;
    }
  }

  //case 10 方法装饰器
  class MyClass1 {
    log(value: string, @parameterDecorator metadata: any) {// arkts-no-ts-decorators
      console.log(Logged: ${value});
    }
  }

  //case 11 访问器装饰器
  class Person {
    private _name: string;
    constructor(name: string) {
      this._name = name;
    }
    @accessorDecorator// arkts-no-ts-decorators
    get name(): string {
      return this._name;
    }
    set name(value: string) {
      this._name = value;
    }
  }

  //case 12 访问器工厂
  class Person1 {
    private _age: number;
    constructor(age: number) {
      this._age = age;
    }
    @readonly1(true)// arkts-no-ts-decorators
    get age(): number {
      return this._age;
    }
    set age(value: number) {
      this._age = value;
    }
  }
}

export function test_ts_boxed_type(testCaseRet: Array) {
  let test_helper = new TestHelper("TEST_TS_BOXED_TYPE");

  test_helper.test(() => {
    return typeof num_boxed === object;
  }, "typeof num_boxed === object");

  test_helper.test(() => {
    return typeof bool_boxed === object;
  }, "typeof bool_boxed === object");

  test_helper.test(() => {
    return typeof str_boxed === object;
  }, "typeof str_boxed === object");

  test_helper.test(() => {
    return typeof bigint_boxed === object;
  }, "typeof bigint_boxed === object");
  testCaseRet.push(test_helper.done());
}

export function test_ts_object_method(testCaseRet: Array) {
  let test_helper = new TestHelper("TEST_TS_OBJECT_METHOD");

  test_helper.test(() => {
    ts_object_method(new Object2())
    ts_object_method_getOwnPropertyDescriptor(new Object2())
    ts_object_method_getOwnPropertyDescriptors(new Object2())
    ts_object_method_getOwnPropertyNames(new Object2())
    ts_object_method_hasOwn(new Object2())
    ts_object_method_isExtensible(new Object2())
    ts_object_method_isFrozen(new Object2())
    ts_object_method_isSealed(new Object2())
    ts_object_method_keys(new Object2())
    ts_object_method_keys(new Object2())
    ts_object_method_values(new Object2())
    return true;
  }, "true");

  test_helper.test(() => {
    interface Iface {
      a:number
    }
    let a1:Iface = {a:1}
    ts_object_method(a1)
    ts_object_method_getOwnPropertyDescriptor(a1)
    ts_object_method_getOwnPropertyDescriptors(a1)
    ts_object_method_getOwnPropertyNames(a1)
    ts_object_method_hasOwn(a1)
    ts_object_method_isExtensible(a1)
    ts_object_method_isFrozen(a1)
    ts_object_method_isSealed(a1)
    ts_object_method_keys(a1)
    ts_object_method_keys(a1)
    ts_object_method_values(a1)
    return true;
  }, "true");

  test_helper.test(() => {
    ts_object_method(interObj)
    return true;
  }, "false");

  testCaseRet.push(test_helper.done());
}

class Reflect2 {
  a: string = 'hello'
  getName() { return this.a }
}

// 5 Object内置方法作用在ArkTS对象
// 规则名 arkts-interop-ts2s-ts-object-on-static-instance
export function test_ts_reflect_method(testCaseRet: Array) {
  let test_helper = new TestHelper("TEST_TS_REFLECT_METHOD");
  test_helper.test(() => {
    ts_reflect_method(new Reflect2())
    ts_reflect_method_apply(new Reflect2())
    ts_reflect_method_defineProperty(new Reflect2())
    ts_reflect_method_deleteProperty(new Reflect2())
    ts_reflect_method_getOwnPropertyDescriptor(new Reflect2())
    ts_reflect_method_ownKeys(new Reflect2())
    ts_reflect_method_isExtensible(new Reflect2())
    ts_reflect_method_set(new Reflect2())
    ts_reflect_method_setPrototypeOf(new Reflect2())
    return true;
  }, "reflect class 1.2 ");

  test_helper.test(() => {
    interface Iface {
      a:number
    }
    let a1:Iface = {a:1}
    ts_reflect_method(a1)
    ts_reflect_method_apply(a1)
    ts_reflect_method_defineProperty(a1)
    ts_reflect_method_deleteProperty(a1)
    ts_reflect_method_getOwnPropertyDescriptor(a1)
    ts_reflect_method_ownKeys(a1)
    ts_reflect_method_isExtensible(a1)
    ts_reflect_method_set(a1)
    ts_reflect_method_setPrototypeOf(a1)
    return true;
  }, "reflect interface 1.2");

  test_helper.test(() => {
    ts_reflect_method(interObj)
    ts_reflect_method_apply(interObj)
    ts_reflect_method_defineProperty(interObj)
    ts_reflect_method_deleteProperty(interObj)
    ts_reflect_method_getOwnPropertyDescriptor(interObj)
    ts_reflect_method_ownKeys(interObj)
    ts_reflect_method_isExtensible(interObj)
    ts_reflect_method_set(interObj)
    ts_reflect_method_setPrototypeOf(interObj)
    return true;
  }, "reflect interObj 1.0 ");

  testCaseRet.push(test_helper.done());
}