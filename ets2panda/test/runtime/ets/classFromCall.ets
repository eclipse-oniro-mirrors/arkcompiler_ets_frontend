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


function main(){
  foo(Class.from<Int| String | null | undefined >());
  foo(Class.from<String>());
  foo(Class.from<int>());
  foo(Class.from<undefined>());
  foo(Class.from<never>());
  foo(Class.from<void>());

  InstanceTesting<Object>();
  ArrayInstanceTesting();
}

class C {
    retUnion() : int | string {return 1}
}

class G<T> {}

class A {
  public x: never

  public static y: never = A.staticNeverFunc()

  public constructor() {
    this.x = this.neverFunc()
  }

  public neverFunc(): never {
    throw new Error("")
  }

  public static staticNeverFunc(): never {
    throw new Error("")
  }

  public neverParam(x: never): void {
  }

  public static staticNeverParam(x: never): void {
  }
}

function InstanceTesting<T>(){
  arktest.assertEQ(Class.from<C>(), Class.of(new C))

  arktest.assertEQ(Class.from<null>(), Class.of(null))
  arktest.assertEQ(Class.from<int | string>(), Class.from<C>().getInstanceMethod('retUnion')!.getReturnType())
  arktest.assertEQ(Class.from<string | int>(), Class.from<int | string>())
  arktest.assertEQ(Class.from<int | undefined>(), Class.from<int>())
  arktest.assertEQ(Class.from<C | undefined>(), Class.from<C>())
  arktest.assertNE(Class.from<int>(), Class.from<FixedArray<int | undefined>>())
  arktest.assertNE(Class.from<int>(), Class.from<FixedArray<int>>())
  arktest.assertEQ(Class.from<int>(), Class.from<FixedArray<int | undefined>>().getFixedArrayComponentType())
  arktest.assertEQ(Class.from<int>(), Class.from<FixedArray<int>>().getFixedArrayComponentType())
  arktest.assertEQ(Class.from<G<number>>(), Class.from<G<string>>())
  arktest.assertEQ(Class.from<Array<G<string>>>(), Class.from<Array<int>>())

  arktest.assertEQ(Class.from<undefined>(), Class.from<void>())
  arktest.assertEQ(Class.from<undefined>(), Class.from<never>())
  arktest.assertEQ(Class.from<void>(), Class.from<never>())

  arktest.assertNE(Class.from<never>(), Class.PRIMITIVE_NEVER)

  arktest.assertEQ(Class.from<T>(), Class.from<Any>())

  arktest.assertEQ(Class.PRIMITIVE_NEVER, Class.from<A>().getInstanceMethod("neverFunc")!.getReturnType())
  arktest.assertEQ(Class.PRIMITIVE_NEVER, Class.from<A>().getStaticMethod("staticNeverFunc")!.getReturnType())
  arktest.assertEQ(Class.from<never>(), Class.from<A>().getInstanceMethod("neverParam")!.getParameterType(0))
  arktest.assertEQ(Class.from<never>(), Class.from<A>().getStaticMethod("staticNeverParam")!.getParameterType(0))
  arktest.assertEQ(Class.from<never>(), Class.from<A>().getInstanceField("x")!.getType())
  arktest.assertEQ(Class.from<never>(), Class.from<A>().getStaticField("y")!.getType())
}

function ArrayInstanceTesting(){
  arktest.assertEQ(Class.from<FixedArray<boolean>>(), Class.from<FixedArray<boolean|undefined>>())
  arktest.assertEQ(Class.from<FixedArray<byte>>(), Class.from<FixedArray<byte|undefined>>())
  arktest.assertEQ(Class.from<FixedArray<char>>(), Class.from<FixedArray<char|undefined>>())
  arktest.assertEQ(Class.from<FixedArray<short>>(), Class.from<FixedArray<short|undefined>>())
  arktest.assertEQ(Class.from<FixedArray<int>>(), Class.from<FixedArray<int|undefined>>())
  arktest.assertNE(Class.from<FixedArray<long>>(), Class.from<FixedArray<int|undefined>>())
  arktest.assertNE(Class.from<FixedArray<float>>(), Class.from<FixedArray<int|undefined>>())
  arktest.assertEQ(Class.from<FixedArray<double>>(), Class.from<FixedArray<double|undefined>>())
  arktest.assertEQ(Class.from<FixedArray<number>>(), Class.from<FixedArray<number|undefined>>())
}

function anyMethod(): Any {
  return undefined
}

function foo(a:Class){}
