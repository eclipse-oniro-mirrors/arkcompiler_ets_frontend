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

// any.1 Object type
export let a: any = {
  pa: "a",
  pb: "b",
  log: () => {
    console.info("log a")
  }
};

// any.2 Basic type
export let a1: any = undefined; // Explicit any type

export let a2; // Implicit any type

// any.3 Any array
export let a3: any[] = [1, "string", true, { key: "value" }];
// any.4 Function
export function a4(param: any): any {
  return param;
}
// any.5 Arrow function
export const a5 = (input: any): any => input;

// any.6 Nested
export let a6: Array<any> = ["1",2, {"a":"b"}];

// any.7 Union type
export let a7: any | string = "test";

// any.8 Interface
export interface a8 {
  fn: any;
}

// any.9 Type alias
export type a9 = any | string;

// unknown
export let b: unknown = undefined;
export const b1: unknown = JSON.parse('{"k": "v"}');
export function b2(): unknown {
  return "b2";
}
export const bbb: Array<unknown> = [1,2,3]
export const b3: () => unknown = () => "b3";
export const b4: string | unknown = "b4";
export interface bbbb {
  f: unknown;
}
export type bbbb2 = unknown | string;

// symbol
export const c: symbol = Symbol('c');

// Function
export const d: Function = function (x: number): string {
  return `The number is ${x}`;
};

// object literal
export type ET = { x: number, y: string };
export const e: ET = { x: 5, y: 'hello' };
export const e2 = {x: 5,y: 'hello'};
export const e3: { x: number, y: string } = {x: 3, y: "a"};

// mixing enum
export enum X { a = 0, b = '1' };
export const f: X = X.a;

// call signature
export const g: {(arg: number): string} = (arg) => `The number is ${arg}`;
export type gt = {(arg: number): string}
export const g2 = ():{(arg: number): string}=>{
  return ()=>"test";
}
export function g3():{(arg: number): string}{
  return ()=>"test";
}

// constructor signature
export const h: { new(): Object } = class {
};

// index signature
export const i: { [index: number]: string } = { 0: 'zero', 1: 'one' };

// intersection
type j1 = {x: number};
type j2 = {y: string};
export const j: { x: number } & { y: string } = { x: 5, y: 'hello' };

// keyof
export type K1 = keyof { x: number, y: string };
export interface K2 {
  props: keyof { x: number, y: string };
}
export const k: K1 = "x";

// typeof.1
let tl = { x: 1, y: '' };
export const l: typeof tl = { x: 2, y: 'world' };
export type l1 = typeof tl;
// Assign to other variable
export const l2 = l;
// typeof.2: typeof in union type
export type l3 = string | number | typeof tl;
// typeof.3: typeof in intersection type
export type l4 = { name: string } & typeof tl;
// typeof.4: typeof in array type
export type l5 = Array<typeof tl>;
// typeof.5: typeof in function parameter type
export type l6 = (arg: typeof tl) => void;
// typeof.6: typeof in function return type
export type l7 = () => typeof tl;
// typeof.7: typeof in conditional type
export type l8<T> = T extends typeof String ? true : false;
// typeof.8: More complex conditional type
export type l9<T> = T extends { type: typeof tl } ? { x: 1 } : never;
// typeof.9: Using typeof in mapped type
export type l10 = keyof typeof tl;
// typeof.10: Using typeof in mapped type
export type l11 = {
  [K in keyof typeof tl]: boolean;
};
// typeof.11: typeof in indexed access
export type l12 = (typeof tl)['x'];

// typeof.13: Function parameter directly using typeof
export function l13(c: typeof tl) {}

// typeof.14: Class property directly using typeof
export class l14 {
  prop: typeof tl = { x: 1, y: "2"};
}

// typeof.15: Interface property directly using typeof
export interface l15 {
  setting: typeof tl;
}

// indexed access type
type MyArray = [{ name: "Alice", age: 15 }];

export type IndexedAccessType = MyArray[number];
export type IndexedAliasType = IndexedAccessType

export const m: MyArray[number] = { name: "Alice", age: 15 };
export const m2: IndexedAccessType = { name: "Alice", age: 15 };

// conditional types
export type ConditionalType<T extends 'A' | 'B'> = T extends 'A' ? 'B' : 'A';

export const n: ConditionalType<'A'> = 'B';
export const n2: ConditionalType<'A'> = 'B';

// mapped types
export type MappedType<T> = { [K in keyof T]: T[K] };

export type TXY = { x: number, y: string };

export const o: MappedType<TXY> = { x: 5, y: 'hello' };

// template literal types
export type AB = 'A' | 'B';

export type TemplateType = `${AB}_id`;

export const p: TemplateType = 'A_id';

// Utility types
export const q: Pick<{ x: number, y: string, z: boolean }, 'x' | 'y'> = { x: 5, y: 'hello' };
export type QT = Pick<{ x: number, y: string, z: boolean }, 'x' | 'y'>;
export let qv: QT;

export const r: Omit<{ x: number, y: string, z: boolean }, 'z'> = { x: 5, y: 'hello' };

export const s: Exclude<'A' | 'B' | 'C', 'B'> = 'A';

export const t: Extract<'A' | 'B' | 'C', 'B'> = 'B';

export const u: NonNullable<string | null | undefined> = 'hello';

export const v: Parameters<(x: number, y: string) => void> = [5, 'hello'];

export const w: ConstructorParameters<new (x: number, y: string) => void> = [5, 'hello'];

export const x: ReturnType<() => string> = 'hello';

export const y: InstanceType<new () => { x: number }> = { x: 5 };

export const z: Uppercase<'hello'> = 'HELLO';

export const aa: Lowercase<'HELLO'> = 'hello';

export const ab: Capitalize<'hello'> = 'Hello';

export const ac: Uncapitalize<'Hello'> = 'hello';

// Namespace exporting interface
export namespace NS{
  export interface ns_interface {
    theme: string;
    fontSize: number;
  }
  export type X = string;
}

// Module exporting interface
export module MD {
  export interface md_intereface {
    name: string
  }
  export type X = string;
}
