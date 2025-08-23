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
  obj1: number | string = 0.0;
  obj2: string = 'hello';
  obj3: number | string | boolean = false;
}

class B extends A {
  obj1: string | number = 0.0; // OK
  obj2: string = 'hello'; // OK
  obj3: number | string | boolean = false; // OK
}

class C extends A {
  obj1: string = 'hello'; // MISMATCH/ERROR
  obj2: number = 0.0; // MISMATCH/ERROR
  obj3: number | boolean = false; // MISMATCH/ERROR
}

class D extends A {
  obj1: string | number | boolean = 'hello'; // MISMATCH/ERROR
  obj2: number | string = 0.0; // MISMATCH/ERROR
}

class E { obj: number | string = 1.0; }
class F extends E { obj: number = 1.0; }     // will be flagged on F vs E => MISMATCH/ERROR
class G extends F { obj: number = 1.0; }     // will be checked on G vs F => OK

class A1 {}
class A2 extends A1 {
  obj: number | string = 0.0;
}
class A3 extends A2 {}
class A4 extends A3 {
  obj: number = 0.0; // ERROR/MISMATCH => field type is not matching with obj from base class A2
}

class B1 {
  obj: number | string = 0.0;
}

class B2 extends B1 { // no error
  obj: number | string = 0.0;
  obj2: number | string = 0.0;
  obj3: boolean = false;
  obj4: string = 'hello';
}

interface I1 {
  obj: number | string
}

interface I2 {
  obj2: number | string;
}

interface I3 extends I2 {}

class CI1 implements I1, I2 {
  obj: number = 0.0;    // error
  obj2: number = 0.0; // error
}

class CI2 implements I3 {
  obj2: number = 0.0; // error
}
