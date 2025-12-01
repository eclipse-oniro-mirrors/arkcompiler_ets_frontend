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


class A {
  x: number = 1;
  getX(): number { return this.x; }
}
const a1: A = new A();
const correct: () => number = () => a1.getX();



class B {
  msg: string = "hi";
  constructor() {
    const fn: () => string = () => this.get();
  }
  get(): string { return this.msg; }
}


class C {
  msg: string = "hello";
  say(): string { return this.msg; }
}
const t1: C = new C();
setTimeout(t1.say(), 100);



class D {
  label: string = "click";
  onClick(): string { return this.label; }
}
const btn: D = new D();
const handler: () => string = () => btn.onClick();



class E {
  value: number = 10;
  create(): () => number {
    return () => this.get();
  }
  get(): number { return this.value; }
}



const arrow: () => string = () => "A";
const copiedArrow: () => string = () => arrow();



function f(value: number): number { return value; }
const multi: (val: number) => number = (v: number) => f(v);



const arr: number[] = [1, 2, 3];
const sliceFn: (start: number) => number[] = (start: number) => arr.slice(start);