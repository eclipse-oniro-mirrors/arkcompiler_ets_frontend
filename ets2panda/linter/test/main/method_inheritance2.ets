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
  a: number = 0
}
class B {
  a: number = 0
}
class C {
  a: number = 0
}

class D extends A{
  a: number = 0;
  b: number = 1;
}


// T
class Base3<T> {
  foo<T>(obj: A | B): void {
    console.log("base")
  }
  foo2<T>(obj: A | B): void {
    console.log("base")
  }
  foo3<T>(obj: A | B | C): void {
    console.log("base")
  }
}
// T extends
class Derived3<T> extends Base3<T> {
  foo<T>(obj: A): void {      // (arkts-method-inherit-rule) error
    console.log("Derived:" + obj.a)
  }
  foo2<T>(): void {   // (arkts-method-inherit-rule) error
    console.log("Derived:")
  }
  foo3<T>(obj: A | B): void {   //(arkts-method-inherit-rule) error
    console.log("Derived:")
  }
}

// T interface
interface  BaseI2<T> {
  foo<T>(obj: A | B):void;
  foo2<T>(obj: A ): void;
  foo3<T>(obj: A | B | C): void;
}

// T implements
class DerivedI2<T> implements BaseI2<T> {
  foo<T>(obj: A): void {  // (arkts-method-inherit-rule) error
    console.log("Derived");
  }
  foo2<T>(): void {  // (arkts-method-inherit-rule) error
    console.log("Derived");
  }
  foo3<T>(obj: A | B): void {  // (arkts-method-inherit-rule) error
    console.log("Derived");
  }
}

class Base5 {
  public foo(): A|B {
    console.log("base")
    return new A();
  }
  static foo2(){
    console.log("base")
    // return new A();
  }
  async foo3():  Promise<A> {
    console.log("base")
    return new A();
  }
  static foo4():void{
    console.log("base")
    // return new A();
  }
}

// extends
class Derived5 extends Base5 {
  public foo(): A|B|C{      //  (arkts-method-inherit-rule)
    console.log("Derived:")
    return new A();
  }

  static foo2(): A{      //  (arkts-method-inherit-rule) error
    console.log("Derived:")
    return new A();
  }

  async foo3(): Promise<A|B> {   //  (arkts-method-inherit-rule)
    console.log("Derived:")
    return new A();
  }
  static foo4(): A{      //  (arkts-method-inherit-rule)  error
    console.log("Derived:")
    return new A();
  }
}

interface I{
  pp(key:string,value:string):void
  pp(key:string):void
}

class J implements I{
  pp(key: string,value?:string):void {  // arkts-method-inherit-rule
  }
}

interface  BaseI {
  foo(): A|B ;
  foo2(): void;
  foo3(): A;
}

class Derived2 implements  BaseI {
  foo(): A|B|C{       //  (arkts-method-inherit-rule)
    console.log("Derived:")
    return new A();
  }

  foo2(): A{        //  (arkts-method-inherit-rule)
    console.log("Derived:")
    return new A();
  }

  foo3(): A|B {      //  (arkts-method-inherit-rule)
    console.log("Derived:")
    return new A();
  }
}

let b2:BaseI = new Derived2();
b2.foo();
b2.foo2()
b2.foo3()


class Base32<T> {
  foo<T>(): A|B {
    console.log("base")
    return new A();
  }
  foo2<T>():void{
    console.log("base")
    // return new A();
  }
  foo3<T>():  A {
    console.log("base")
    return new A();
  }
  foo4<T>(){
    console.log("base")
    // return new A();
  }
}

class Derived32<T> extends Base32<T> {
  foo<T>(): A|B|C{      //  (arkts-method-inherit-rule)
    console.log("Derived:")
    return new A();
  }

  foo2<T>(): A{      //  (arkts-method-inherit-rule)
    console.log("Derived:")
    return new A();
  }

  foo3<T>(): A|B {   //  (arkts-method-inherit-rule)
    console.log("Derived:")
    return new A();
  }

  foo4<T>(): A{        //  (arkts-method-inherit-rule)
    console.log("Derived:")
    return new A();
  }
}

interface  BaseI4<T> {
  foo<T>(): A|B ;
  foo2<T>(): void;
  foo3<T>(): A;
}

class Derived4<T> implements  BaseI4<T> {
  foo<T>(): A|B|C{         //  (arkts-method-inherit-rule)
    console.log("Derived:")
    return new A();
  }

  foo2<T>(): A{        //  (arkts-method-inherit-rule)
    console.log("Derived:")
    return new A();
  }

  foo3<T>(): A|B {      //  (arkts-method-inherit-rule)
    console.log("Derived:")
    return new A();
  }
}

class Animal {}
class Dog extends Animal {}
class Base55 {
  public foo(obj: Animal): void {
    console.log("base")
  }
}

class Derived55 extends Base55 {
  public foo(obj: Dog): void {                // (arkts-method-inherit-rule)
    console.log("Derived:")
  }
}

class Base6 {
  public foo(): Dog {
    console.log("base")
    return new Dog();
  }
}

class Derived6 extends Base6 {
  public foo(): Animal {      //  (arkts-method-inherit-rule)
    console.log("Derived:")
    return new Animal()
  }
}

abstract class Base66{
  abstract foo();
  abstract foo1();
}

abstract class Derived66 extends Base66{
  async foo(){     // (arkts-method-inherit-rule)

  }
  foo1(): number {    // (arkts-method-inherit-rule)?
    return 1
  }
}
class Base7{
  foo(){

  }
}
class Derived7 extends Base7{
  foo(): number {     // (arkts-method-inherit-rule)
    return 12;
  }
}
class Base44 {
  public foo(obj: A | B): void {
    console.log("base")
  }
  protected foo2(obj: A | B): void {
    console.log("base")
  }
  async foo3(obj: A | B | C): Promise<void> {
    console.log("base")
  }
}

class Derived44 extends Base44 {
  public foo(obj: A): void {              // (arkts-method-inherit-rule)
    console.log("Derived:" + obj.a)
  }
  protected foo2(): void {               // (arkts-method-inherit-rule)
    console.log("Derived:")
  }
  async foo3(obj: A | B): Promise<void>  {               // (arkts-method-inherit-rule)
    console.log("Derived:")
  }
}
class Base {
  foo(obj: A | B): void {
    console.log("base")
  }
  foo2(obj: A | B): void {
    console.log("base")
  }
  foo3(obj: A | B | C): void {
    console.log("base")
  }
  foo4(obj: A): void {
    console.log("base" );
  }
}
class Derived extends Base {
  foo(obj: A): void {              // (arkts-method-inherit-rule)
    console.log("Derived:" + obj.a)
  }
  foo2(): void {               // (arkts-method-inherit-rule)
    console.log("Derived:")
  }
  foo3(obj: A | B): void {              // (arkts-method-inherit-rule)
    console.log("Derived:")
  }
  foo4(obj: D): void {              // (arkts-method-inherit-rule)
    console.log("Derived:" + obj.b)
  }
}
class Derived_Derived extends Derived {
  foo(): void {                // (arkts-method-inherit-rule)
  }

  foo3(obj: A): void {               // (arkts-method-inherit-rule)
    console.log("Derived:")
  }
}

let b:Base = new Derived();
b.foo(new B());
b.foo2(new B())

interface  BaseI22 {
  foo(obj: A | B):void;
  foo2(obj: A ): void;
  foo3(obj: A | B | C): void;
}

class Derived22 implements BaseI22 {
  foo(obj: A): void {                  // (arkts-method-inherit-rule)
    console.log("Derived");
  }
  foo2(): void {             // (arkts-method-inherit-rule)
    console.log("Derived");
  }
  foo3(obj: A | B): void {             // (arkts-method-inherit-rule)
    console.log("Derived");
  }
}
let b22 :BaseI = new Derived2();