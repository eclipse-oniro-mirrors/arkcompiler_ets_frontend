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

let propertyDecorator: PropertyDecorator = (ttt, yyy) => {
  console.log(yyy as string);
}

class A {
  @propertyDecorator
  prop = 1
}

let classDecorator: ClassDecorator = (target) => { return target }

@classDecorator
class B {}

let methodDecorator: MethodDecorator = (target, propertyKey, descriptor) => {
console.log(JSON.stringify(descriptor));
}

class C {
  @methodDecorator
  method(): void {}
}

let paramDecorator: ParameterDecorator = (target, methodName, paramIdx) => {
  console.log(methodName as string);
}

class D {
  method(@paramDecorator param: string): void {}
}

class C implements Iterator<number|undefined> {
  a: number = 0;
  b: number;
  constructor (b:number) {
    this.b = b;
  }
  next(): IteratorResult<number> {
    if (this.a >= this.b) {
      return {value:undefined, done:true}
    }
    this.a++;
    return {value:this.a, done:false}
  }
  return(): IteratorResult<number> { //error
    console.log("call return")
    this.a = 0;
    return {value:undefined, done:true}
  }
  throw(e:Error): IteratorResult<number> { //error
    console.error("Error: " + e);
    return { value: undefined, done: true };
  }
}
let a = new C(5)
a.next()
a.return()
a.throw(new Error("test"))


let b = String.raw`a\nb\nc` //error


function f(c:TemplateStringsArray, ...values: Object[]) {
  c.raw; //error
}
const c: string = "123"
f`Hello, ${c}!`


type T1 = number | T1[];
let arr1: ReadonlyArray<T1> = [1, [1]];
arr1.flat(2); //error


type T = number | T[];
let arr: ReadonlyArray<T> = [1, [1]];
arr.flatMap((value, index, array) => { //error
  return value;
});


let num = new Object();
num.valueOf(); //error


type T2 = number | T2[];
let arr2: Array<T2> = [1, [1]];
arr2.flat(2); //error


const str = '';
const arr3 = Array.from(str); //error


let buff: ArrayBufferTypes = {  
  ArrayBuffer: new ArrayBuffer(10),
  SharedArrayBuffer: new SharedArrayBuffer(10)
}
buff.ArrayBuffer; //error
buff.SharedArrayBuffer;


function fn1(iArguments: IArguments) {
  Reflect.get(iArguments, Symbol.iterator); //error
}


function fn2(iArguments: IArguments) {
  iArguments.callee; //error
}


function fn3(iArguments: IArguments) {
  iArguments.length; //error
}


let o: Intl.DateTimeFormatPartTypesRegistry = {
  day: undefined, //error
  dayPeriod: undefined, //error
  era: undefined, //error
  hour: undefined, //error
  literal: undefined, //error
  minute: undefined, //error
  month: undefined, //error
  second: undefined, //error
  timeZoneName: undefined, //error
  weekday: undefined, //error
  year: undefined, //error
  unknown: undefined,
  fractionalSecond: undefined
}


Intl.DisplayNames.prototype; //error


Intl.ListFormat.prototype;  //error


Intl.NumberFormat.prototype;  //error


const arr4 = [0, 1, 2, [3, 4]];
let arr5: FlatArray<number[], -1> = arr4.flat();  //error


let d: Object = Object(); //error


let d1: Object = Object({}); //error


const text = '2025-01-01';
const regex = /(?<year>\d{4})-(?<month>\d{2})-(?<day>\d{2})/;
const match = regex.exec(text);
console.log(match!.groups!.year);  //error


const text1 = '2025-01-01';
const regex1 = /(?<year>\d{4})-(?<month>\d{2})-(?<day>\d{2})/;
const match1 = text1.match(regex1);
console.log(match1!.groups!.year);  //error


class FileReader {
  private lines: string[] = ["Line 1", "Line 2", "Line 3"];

  [Symbol.iterator](): Iterator<string> {
    let index = 0;
    const self = this;

    return {
      next(): IteratorResult<string> {
        if (index < self.lines.length) {
          return { value: self.lines[index++], done: false };
        } else {
          return { value: undefined, done: true };
        }
      },

      return(): IteratorReturnResult<number> { //error
        return { done: true, value: index };
      },
    };
  }
}


class Counter {
  private count: number = 0;
  private max: number;

  constructor(max: number) {
    this.max = max;
  }

  [Symbol.iterator](): Iterator<number> {
    const self = this;

    return {
      next(): IteratorYieldResult<number> { //error
        if (self.count < self.max) {
          return {
            value: self.count++,
          };
        }

        return { done: false, value: -1 };
      },
    };
  }
}
