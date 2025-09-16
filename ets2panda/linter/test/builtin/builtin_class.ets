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

class MyIntArray extends Int8Array implements Int8Array{} //BuiltinFinalClass*2
interface MyIntArray2 extends Int8Array {} //BuiltinFinalClass

class MyBigInt1 extends BigInt implements BigInt { //BuiltinFinalClass*2
  constructor(value: bigint | string | number) {
    super(value);
  }
  [Symbol.iterator](): IterableIterator<BigInt>{ //BuiltinSymbolIterator
    return this;
  }
}

class MyDataView1 extends DataView implements  DataView{} //BuiltinFinalClass*2
interface MyDataView3 extends DataView {} //BuiltinFinalClass

class MyInt16Array1 extends Int16Array implements Int16Array { //BuiltinFinalClass*2
  filter(predicate: (value: number, index: number, array: Int16Array) => number, thisArg?: number): Int16Array {//BuiltinAll
    return new Int16Array;
  }
}
interface MyInt16Array2 extends Int16Array{ //BuiltinFinalClass
  every(predicate: (value: number, index: number, array: Int16Array) => void, thisArg?: string): void;
}
class MyInt32Array1 extends Int32Array{}//BuiltinFinalClass
class MyInt32Array2 implements Int32Array{} //BuiltinFinalClass
interface MyInt32Array3 extends Int32Array{}; //BuiltinFinalClass

class MyBigInt64Array1 extends BigInt64Array implements BigInt64Array{}//BuiltinFinalClass*2
interface  MyBigInt64Array2 extends BigInt64Array{}//BuiltinFinalClass

class EmptyIterator<T> implements IterableIterator<T>{//
  next(): IteratorResult<T> {//BuiltinAll
    return { done: true, value: undefined as void as T };
  }

  [Symbol.iterator](): IterableIterator<T> {//BuiltinSymbolIterator*2
    return this;
  }}
interface  EmptyIterator2<MyBigInt64Array1> extends  IterableIterator<MyBigInt64Array1>{}

class MyFloatArray1 extends Float32Array implements Float32Array,Float64Array {}  //BuiltinFinalClass*3
interface MyFloat32Array3 extends Float32Array{} //BuiltinFinalClass
interface MyFloat64Array3 extends Float64Array{} //BuiltinFinalClass

class MyWeak<K extends object, V> extends WeakMap<K, V>implements WeakMap<K, V>,WeakSet<Number>{} //BuiltinFinalClass*3
interface MyWeakMap3<K extends object, V> extends WeakMap<K, V>{} //BuiltinFinalClass
class MyWeakSet1<T extends object> extends WeakSet<T>{} //BuiltinFinalClass
interface MyWeakSet3<T extends object> extends WeakSet<T>{} //BuiltinFinalClass

class MyObject extends Boolean implements Boolean,String{} //BuiltinFinalClass*3
interface MyBoolean3 extends Boolean {} //BuiltinFinalClass
interface MyString3 extends String{}  //BuiltinFinalClass
class MyFinalizationRegistry1<T> extends FinalizationRegistry<T>{} //BuiltinFinalClass
class MyFinalizationRegistry2<T> implements FinalizationRegistry<T> {  //BuiltinFinalClass
  [Symbol.toStringTag]: "FinalizationRegistry"|undefined=undefined;
  register(target: object, heldValue: T, unregisterToken?: object): void {}
  unregister(unregisterToken: object): void {}
}
interface MyFinalizationRegistry3<T> extends FinalizationRegistry<T>{} //BuiltinFinalClass
class MyPromise1<T> extends Promise<T> {
  constructor(executor: (resolve: (value: T | PromiseLike<T>) => void, reject: (reason?: number) => void) => void) {
    super(executor);
  }
}

class MyPromise2<T> implements Promise<T> {
  then<TResult1 = T, TResult2 = never>(
    onfulfilled?: ((value: string) => TResult1 | PromiseLike<TResult1>) | null,
    onrejected?: ((reason: string) => TResult2 | PromiseLike<TResult2>) | null
  ): Promise<TResult1 | TResult2> {
    return Promise.resolve() as Promise<TResult1 | TResult2>;
  }

  catch<TResult = never>(
    onrejected?: ((reason: string) => TResult | PromiseLike<TResult>) | null
  ): Promise<T | TResult> {
    return Promise.resolve() as Promise<T | TResult>;
  }

  finally(onfinally?: (() => void) | null): Promise<T> {
    return Promise.resolve() as Promise<T>;
  }

  [Symbol.toStringTag]: string = "Promise";
}

interface MyPromise3<T> extends Promise<T> {}
