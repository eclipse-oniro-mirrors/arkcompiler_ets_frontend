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


function InstanceTesting<T>(){
  arktest.assertEQ(Class.from<C>(), Class.of(new C))
  
  arktest.assertEQ(Class.from<null>(), Class.of(null))
  arktest.assertEQ(Class.from<int | string>(), Class.from<C>().getInstanceMethodByName('retUnion')!.getReturnType())
  arktest.assertEQ(Class.from<string | int>(), Class.from<int | string>())
  arktest.assertEQ(Class.from<int | undefined>(), Class.from<int>())
  arktest.assertEQ(Class.from<C | undefined>(), Class.from<C>())
  arktest.assertNE(Class.from<int>(), Class.from<FixedArray<int | undefined>>())
  arktest.assertNE(Class.from<int>(), Class.from<FixedArray<int>>())
  arktest.assertEQ(Class.from<int>(), Class.from<FixedArray<int | undefined>>().getFixedArrayComponentType())
  arktest.assertNE(Class.from<int>(), Class.from<FixedArray<int>>().getFixedArrayComponentType())
  arktest.assertEQ(Class.from<G<number>>(), Class.from<G<string>>())
  arktest.assertEQ(Class.from<Array<G<string>>>(), Class.from<Array<int>>())
  
  arktest.assertEQ(Class.from<undefined>(), Class.from<Any>())
  arktest.assertEQ(Class.from<void>(), Class.from<Any>())
  arktest.assertEQ(Class.from<T>(), Class.from<Any>())
  arktest.assertEQ(Class.from<never>(), Class.from<Object>())
}

function ArrayInstanceTesting(){
  arktest.assertNE(Class.from<FixedArray<boolean>>(), Class.from<FixedArray<boolean|undefined>>())
  arktest.assertNE(Class.from<FixedArray<byte>>(), Class.from<FixedArray<byte|undefined>>())
  arktest.assertNE(Class.from<FixedArray<char>>(), Class.from<FixedArray<char|undefined>>())
  arktest.assertNE(Class.from<FixedArray<short>>(), Class.from<FixedArray<short|undefined>>())
  arktest.assertNE(Class.from<FixedArray<int>>(), Class.from<FixedArray<int|undefined>>())
  arktest.assertNE(Class.from<FixedArray<long>>(), Class.from<FixedArray<int|undefined>>())
  arktest.assertNE(Class.from<FixedArray<float>>(), Class.from<FixedArray<int|undefined>>())
  arktest.assertNE(Class.from<FixedArray<double>>(), Class.from<FixedArray<double|undefined>>())
  arktest.assertNE(Class.from<FixedArray<number>>(), Class.from<FixedArray<number|undefined>>())
}

function anyMethod(): Any {
  return undefined
}

function foo(a:Class){}
