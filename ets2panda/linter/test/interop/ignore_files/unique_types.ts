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

// any 类型
let any_var: any = 10;
any_var = "string";
export {any_var}

// unknown类型
export let unknown_var:unknown = 10;

// Symbol类型
export let symbol_var: symbol = Symbol("description");

// Function类型
export let function_var: Function = function() {
  console.log("Hello, World!");
  return true;
};

//mixed enum
export enum enum_var {
    a = 0,
    b = '1',
}

export class A {
  static instance = new A();
}

export interface B {
  name:string
}

//用例一
export function throw_number() {
  throw 123;
}

export function throw_string() {
  throw "error";
}

export function throw_boolean() {
  throw true;
}

export function throw_bigint() {
  throw 111n;
}

export function throw_obj() {
  throw { name:'error' };
}

export function throw_error() {
  throw new Error("error");
}

export class SubError extends Error {
  extraField: number = 123;
  foo() { return 456; }
}

export function throwErrorSubClass() {
  throw new SubError();
}
export function throwRangeError() {
  throw new RangeError();
}

//用例二
export class ObjectLiteralClass {
  name:string = ""
}

export interface ObjectLiteralInter {
  name:string
}
//用例三
//类装饰器
export function MyClassDecorator(klass: Object) {}

//属性装饰器
export function propertyDecorator(target: Object, propertyKey: string) {
  console.log(Property Decorator called on: ${target.constructor.name} with propertyKey: ${propertyKey});
}

//方法装饰器
export function methodDecorator(target: Object, propertyKey: string , descriptor: PropertyDescriptor) {
  console.log(Method Decorator called on: ${target.constructor.name} with propertyKey: ${propertyKey});
  descriptor.value = function (...args: any[]) {
    console.log(Called: ${propertyKey} with, args);
    return args[0] * 2;
  };
  return descriptor;
}

// 参数装饰器
export function parameterDecorator(target: Object, propertyKey: string , parameterIndex: number) {
  console.log(Parameter Decorator called on: ${target.constructor.name} with propertyKey: ${propertyKey} and parameterIndex: ${parameterIndex});
}

// 访问器装饰器
export function accessorDecorator(target: any, propertyKey: string, descriptor: PropertyDescriptor) {
  const originalGet = descriptor.get;
  const originalSet = descriptor.set;

  descriptor.get = function() {
    console.log(Getter for ${propertyKey} called.);
    return originalGet.apply(this);
  };
  descriptor.set = function(value) {
    console.log(Setter for ${propertyKey} called with value: ${value});
    originalSet.apply(this, [value]);
  };
  return descriptor;
}

// 装饰器工厂
export function readonly1(isReadonly: boolean) {
  return function (target: any, propertyKey: string, descriptor: PropertyDescriptor) {
    if (isReadonly) {
      descriptor.writable = false;
    }
  };
}

export let num_boxed = new Number(123)
export let bool_boxed = new Boolean(true)
export let str_boxed = new String('hello')
export let bigint_boxed = BigInt(123n)

export function ts_object_method(prx: any) {
  Object.getOwnPropertyDescriptor(prx, "a") // arkts-interop-ts2s-ts-object-on-static-instance * 2 + arkts-limited-stdlib not undefined
  Object.getOwnPropertyDescriptors(prx) // arkts-interop-ts2s-ts-object-on-static-instance * 2 not {}
  Object.getOwnPropertyNames(prx) // arkts-interop-ts2s-ts-object-on-static-instance * 2 ["a"]
  Object.hasOwn(prx, "a") // true
  Object.isExtensible(prx) // arkts-interop-ts2s-ts-object-on-static-instance * 2 true
  Object.isFrozen(prx) // arkts-interop-ts2s-ts-object-on-static-instance * 2 false
  Object.isSealed(prx) // arkts-interop-ts2s-ts-object-on-static-instance * 2 false
  Object.keys(prx) // arkts-interop-ts2s-ts-object-on-static-instance * 2 ["a"]
  Object.setPrototypeOf(prx, {}) // arkts-interop-ts2s-ts-object-on-static-instance * 2 + arkts-limited-stdlib OK
  Object.values(prx) // arkts-interop-ts2s-ts-object-on-static-instance * 2 + arkts-no-inferred-generic-params [1]
  prx.hasOwnProperty("a") // true
  prx.propertyIsEnumerable("a") // true
}

export function ts_object_method_getOwnPropertyDescriptor(prx: any) {
  return Object.getOwnPropertyDescriptor(prx, "a") // arkts-interop-ts2s-ts-object-on-static-instance * 2 + arkts-limited-stdlib not undefined
}
export function ts_object_method_getOwnPropertyDescriptors(prx: any) {
  return Object.getOwnPropertyDescriptors(prx)// arkts-interop-ts2s-ts-object-on-static-instance * 2 + arkts-limited-stdlib not {}
}
export function ts_object_method_getOwnPropertyNames(prx: any) {
  return Object.getOwnPropertyNames(prx) // arkts-interop-ts2s-ts-object-on-static-instance * 2 ["a"]
}
export function ts_object_method_hasOwn(prx: any) {
  return Object.hasOwn(prx, "a") // true
}
export function ts_object_method_isExtensible(prx: any) {
  return Object.isExtensible(prx) // arkts-interop-ts2s-ts-object-on-static-instance * 2 + arkts-limited-stdlib true
}
export function ts_object_method_isFrozen(prx: any) {
  return Object.isFrozen(prx) // arkts-interop-ts2s-ts-object-on-static-instance * 2 + arkts-limited-stdlib false
}

export function ts_object_method_isSealed(prx: any) {
  return Object.isSealed(prx) // arkts-interop-ts2s-ts-object-on-static-instance * 2 + arkts-limited-stdlib false
}

export function ts_object_method_keys(prx: any) {
  return Object.keys(prx) // arkts-interop-ts2s-ts-object-on-static-instance * 3 ["a"]
}

export function ts_object_method_setPrototypeOf(prx: any) {
  return Object.setPrototypeOf(prx, {}) // arkts-limited-stdlib OK
}

export function ts_object_method_values(prx: any) {
  return Object.values(prx) // arkts-interop-ts2s-ts-object-on-static-instance * 2 + arkts-no-inferred-generic-params [1]
}

interface Iface {
  a:number
}
export let interObj:Iface = {a:1}

export function ts_reflect_method(prx: any) {
  Reflect.apply(prx.getName, {a: 12}) // arkts-interop-ts2s-ts-object-on-static-instance * 2 12
  Reflect.defineProperty(prx, 'newField', {value: 7}) // arkts-interop-ts2s-ts-object-on-static-instance * 2 true
  Reflect.deleteProperty(prx, "a") // arkts-interop-ts2s-ts-object-on-static-instance * 2 true
  Reflect.getOwnPropertyDescriptor(prx, "a") // arkts-interop-ts2s-ts-object-on-static-instance * 2 not undefined
  Reflect.ownKeys(prx) // arkts-interop-ts2s-ts-object-on-static-instance * 2 ['a']
  Reflect.isExtensible(prx) // arkts-interop-ts2s-ts-object-on-static-instance * 2 true
  Reflect.set(prx, 'newField', 7) // arkts-interop-ts2s-ts-object-on-static-instance * 2 true
  Reflect.setPrototypeOf(prx, {}) // arkts-interop-ts2s-ts-object-on-static-instance * 2 true
}

export function ts_reflect_method_apply(prx: any) {
  Reflect.apply(prx.getName, {a: 12}) // arkts-interop-ts2s-ts-object-on-static-instance * 2 12
}
export function ts_reflect_method_defineProperty(prx: any) {
  Reflect.defineProperty(prx, 'newField', {value: 7}) // arkts-interop-ts2s-ts-object-on-static-instance * 2 true
}
export function ts_reflect_method_deleteProperty(prx: any) {
  Reflect.deleteProperty(prx, "a") // arkts-interop-ts2s-ts-object-on-static-instance * 2 true
}
export function ts_reflect_method_getOwnPropertyDescriptor(prx: any) {
  Reflect.getOwnPropertyDescriptor(prx, "a") // arkts-interop-ts2s-ts-object-on-static-instance * 2 not undefined
}
export function ts_reflect_method_ownKeys(prx: any) {
  Reflect.ownKeys(prx) // arkts-interop-ts2s-ts-object-on-static-instance * 2 ['a']
}
export function ts_reflect_method_isExtensible(prx: any) {
  Reflect.isExtensible(prx) // arkts-interop-ts2s-ts-object-on-static-instance * 2 true
}
export function ts_reflect_method_set(prx: any) {
  Reflect.set(prx, 'newField', 7) // arkts-interop-ts2s-ts-object-on-static-instance * 2 true
}
export function ts_reflect_method_setPrototypeOf(prx: any) {
  Reflect.setPrototypeOf(prx, {}) // arkts-interop-ts2s-ts-object-on-static-instance * 2 true
}