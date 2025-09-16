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

// any
let any_var: any = 10;
any_var = "string";
export { any_var }

// unknown
export let unknown_var: unknown = 10;

// Symbol
export let symbol_var: symbol = Symbol("description");

// Function
export let function_var: Function = function () {
    console.log("Hello, World!");
    return true;
};

// objectLiteral
export let objectLiteral_var: { x: number, y: string } = { x: 10, y: "hello" };
export let objectLiteral_var2 = { x: 10, y: "hello" };
export let objectLiteral_var3:{};
export let objectLiteral_var32={};
export let objectLiteral_var4 = { x: 1 };
export function objectLiteral_var5() {
    return { x: 1 };
}
export function objectLiteral_var51(aa:{}):{}{
    return { x: 1 };
}

export class objectLiteral_var6 {
    get() { return { a: '1' }; }
    get2():{} { return {}; }
    get3() { return {}; }
    get4():{}|undefined {return; }
    get1(): { x: number }|undefined {
        return;
    }
    set(aa: { x: 1 }) { }
    set3(aa: {}) { }
    set1(aa: { x: 1 }|boolean) { }
    set2(aa: { x: 1,y: string }|boolean,bb:string) { }
}
// enum
export enum enum_var {
    a = 0,
    b = '1',
}

// function type
export type func_type = (arg: number) => string;
export type func_type2 = {(arg: number): string};

// Construct signature
export let constructor_type: { new(name: string): { name: string } } = class {
    constructor(public name: string) { }
};

// Index Signature
let objIndexSignature_var: { [index: number]: string } = {};
objIndexSignature_var[0] = "zero";
objIndexSignature_var[1] = "one";
export { objIndexSignature_var }

// Intersection
interface TypeA {
    a: number;
}
interface TypeB {
    b: string;
}

export let Intersection_obj: TypeA & TypeB = { a: 10, b: "hello" };

// keyof
export interface X {
    a: number;
    b: string;
}

export type KeyOf_Type = keyof X;
export let de: KeyOf_Type;
export let key: keyof X;
export function keyFuns(){
    return key;
}
export class Keys{
    keys:keyof X;
    set(x:keyof X|undefined){}
    get():keyof X|void{}
    get1(){ return this.keys}
}

// typeof 
let p = { x: 1, y: "" };
export let typeOf_type = typeof p;
export let typeOf_type1: typeof p;
// let q: typeof p = { x: 2, y: "world" };

export let user = { name: "John", age: 25 };

export type SomeType = {
    name: string,
}
//indexed access: type[index]。
const MyArray = [{ name: "Alice", age: 15 }]
export type Person = typeof MyArray[number]
export let Person1 = MyArray[0]
type StringArray = string[];
export type ElementType = StringArray[number];
type Demo = {
    id: number;
    name: string;
    address: { city: string; };
};
type NameType = Demo["name"];
type CityType = Demo["address"]["city"];
export type NameOrAddress =Demo["name" | "address"];
export function getInfo(name:NameType){
    return name as CityType;
}
type Tuple = [string, number, boolean];
type FirstElement = Tuple[0]; 
type LastElement = Tuple[2];
export class IndexAccess{
    par:Person;
    set(city:CityType,NameOrAddr:NameOrAddress){}
    get(name:NameType):Tuple|FirstElement|LastElement{
        return getInfo(name);
    }
}
type UserKeys = keyof Demo;
export type UserValueTypes = Demo[UserKeys];

//Template Literal Types:${T}${U}...
type UnionString = "A" | "B";
export type TemplateLiteralType =` ${UnionString}_id`;
type Direction = "up" | "down" | "left" | "right";
type Action = "move" | "rotate";
export type Commands = `${Action}_${Direction}`;
type PropEventNames<T extends string> = `${T}Changed`;
export type NameChanged = PropEventNames<"name">;
type Colors1 = "red" | "blue";
type Sizes = "small" | "large";
export type Variants = `${Colors1}_${Sizes}`;
type IsString<T> = T extends string ? true : false;
export type Check<T> = `${T}` extends string ? true : false;
export function testTemplateLiteralType(bb:Commands){
    let a :PropEventNames<"name">;
}

export let objectLiteralType: SomeType;
export let mixedEnumType: X;
export let intersectionType: SomeType & X;
export let templateLiteralType: TemplateLiteralType|undefined;

export function tsFunction() { };
export let stringType: string;
export class Test{
    returnStr(){
        return stringType;
    }
}
//conditional types：T extends U ? X : Y
type ExtractNumber<T> = T extends number ? T : never;
export type NumbersOnly = ExtractNumber<string | number | boolean>;
export type ReturnVal<T extends (...args: any[]) => any> =
    T extends (...args: any[]) => infer R ? R : never;
export type Num = ReturnVal<() => number>;

//mapped types:{[K in KeyType]: TypeExpression}
type Readonly<T> = {
    readonly [K in keyof T]: T[K];
};
export type ReadonlyUser = Readonly<User>;
export type Partial<T> = {
    [K in keyof T]?: T[K];
};
export type OptionalUser = Partial<User>;
type Original = {
    [key: string]: number;
    name: string;
};
export type Mapped = {
    [K in keyof Original]: Original[K];
};

export let sumType: { [index: number]: string ,temp:TemplateLiteralType,index:UserValueTypes} ;
//tool
type User = {
    name: string;
    age: number;
    email: string;
};
type Colors = 'red' | 'blue' | 'green' | 'yellow';
// Pick<Type, Keys>
export type NameAndEmail = Pick<User, 'name' | 'email'>;
// Omit<Type, Keys>
export type WithoutEmail = Omit<User, 'email'>;
// Exclude<UnionType, ExcludedMembers>
export type PrimaryColors = Exclude<Colors, 'green' | 'yellow'>;
// Extract<Type, Union>
type Values = string | number | boolean | null;
export type PrimitiveValues = Extract<Values, string | number | boolean>;
// NonNullable
type Data = string | null | undefined;
export type CleanData = NonNullable<Data>;
// Parameters
function add(a: number, b: string): void { }
export type AddParams = Parameters<typeof add>;
// ConstructorParameters
class Per {
    constructor(name: string, age: number) { }
}
export type PerParams = ConstructorParameters<typeof Per>;
// ReturnType
function createUser(): { id: number; name: string } {
    return { id: 1, name: 'Alice' };
}
export type UserType = ReturnType<typeof createUser>;
// InstanceType
class Point {
    x: number;
    y: number;
}
export type PointInstance = InstanceType<typeof Point>;
// NoInfer
type NoInfer<T> = [T][T extends any ? 0 : never];
export function identity<T>(x: T, y: NoInfer<T>): T {
    return x;
}
// ThisParameterType
type getThis = (this: { name: string }) => void;
export type thisParameterType = ThisParameterType<getThis>;
// OmitThisParameter
type GetThis = (this: { name: string }, x: number) => void;
export type WithoutThis = OmitThisParameter<GetThis>;
// ThisType
export type ClassThisType = ThisType<{
    getName(): string;
}>;
// Uppercase
type Greeting = 'hello';
export type ShoutGreeting = Uppercase<Greeting>;
// Lowercase
export type QuietGreeting = Lowercase<Greeting>;
// Capitalize
export type CapitalizedWord = Capitalize<Greeting>;
// Uncapitalize
export type UncapitalizedWord = Uncapitalize<Greeting>;
export function getCapitalize(): Capitalize<Greeting> {
    return 'Hello';
}