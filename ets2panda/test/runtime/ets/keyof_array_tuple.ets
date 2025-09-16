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
  readonly field1:("abcd"|(keyof A)|number|A)[] = ["abcd","field1","foo",123,new A()];
}

type keyofA = keyof A;

function main():void{
  let a:A = new A();

  let x:("abcd"|keyofA|number|A)[] = ["abcd","field1","foo",123,a];
  let x2:("abcd"|(keyof A)|number|A)[] = ["abcd","field1","foo",123,a];
  let x3:("abcd"|(keyof Number)|number|A)[] = ["abcd","valueOf","toPrecision",123,a];

  let x4:["abcd",keyofA,number,A] = ["abcd","field1",123,a];
  let x5:["abcd",(keyof A),number,A] = ["abcd","field1",123,a];
  let x6:["abcd",(keyof Number),number,A] = ["abcd","valueOf",123,a];

  let x7:("abcd"|keyofA|number|A)[][] = [["abcd","field1","foo",123,a],["abcd","field1","foo",123,a]];
  let x8:("abcd"|(keyof A)|number|A)[][] = [["abcd","field1","foo",123,a],["abcd","field1","foo",123,a]];
  let x9:("abcd"|(keyof Number)|number|A)[][] = [["abcd","valueOf","toPrecision",123,a],["abcd","valueOf","toPrecision",123,a]];

  let x10:Array<"abcd"|keyofA|number|A> = new Array<"abcd"|keyofA|number|A>("abcd","field1","foo",123,a);
  let x11:Array<"abcd"|(keyof A)|number|A> = new Array<"abcd"|(keyof A)|number|A>("abcd","field1","foo",123,a);
  let x12:Array<"abcd"|(keyof Number)|number|A> = new Array<"abcd"|(keyof Number)|number|A>("abcd","valueOf","toPrecision",123,a);
}
