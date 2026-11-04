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

class A{
  field1:number = 10;
  foo(){}
}

class B{
  field1:keyof A = "field1";
}

interface I{
  field1:keyof A;
}

function main():void{
  let x1:B = {field1:"field1"};
  let x2:Partial<B> = {field1:"field1"};
  let x3:Partial<B> = {field1:"foo"};
  let x4:Partial<B> = {};
  let x5:Required<B> = {field1:"field1"};
  let x6:Required<B> = {field1:"foo"};
  let x7:Readonly<B> = {field1:"field1"};
  let x8:Readonly<B> = {field1:"foo"};

  let x9:B = new B();
  x9.field1 = "field1";
  x9.field1 = "foo"

  let i1:I = {field1:"field1"};
  let i2:Partial<I> = {field1:"field1"};
  let i3:Partial<I> = {field1:"foo"};
  let i4:Partial<I> = {};
  let i5:Required<I> = {field1:"field1"};
  let i6:Required<I> = {field1:"foo"};
  let i7:Readonly<I> = {field1:"field1"};
  let i8:Readonly<I> = {field1:"foo"};
}
