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

class A1 {
  toString(): string { // valid
    return "ok";
  }

  toLocaleString(): string { // valid
    return "ok";
  }
  
  valueOf(): number { // error
    return 789;
  }

  hasOwnProperty(): boolean { // error
    return false;
  }

  isPrototypeOf(v: Object): boolean { // valid
    return true;
  }

  propertyIsEnumerable(v: PropertyKey): boolean { // valid
    return false;
  }
}

class A2 {
  customFunc(): void {} // valid
  
  toString(): number {return 123;} // error

  toLocaleString(): number { // error
    return 456;
  }

  valueOf(): Object { // valid
    return {};
  }
  
  hasOwnProperty(v: PropertyKey): boolean { // valid
    return true;
  }

  isPrototypeOf(v: string): boolean { // error
    return false;
  }

  propertyIsEnumerable(v: PropertyKey): string { // error
    return "not boolean";
  }
}

class A3 {
  toString(): string { return "ok"; } // valid
  valueOf(): number { return 123; } // error
}

class A4 {
  customFunc(): void {} // valid
  valueOf(): number { return 123; } // error
  toString(): string { return "ok"; } // valid
}

interface I1 {
  toString(): string; // valid
  toLocaleString(): number; // error
  valueOf(): number; // error
  customFunc(): boolean; // valid
  hasOwnProperty(v: PropertyKey): boolean; // valid
  isPrototypeOf(v: Object): boolean; // valid
  propertyIsEnumerable(v: PropertyKey): boolean; // valid
}

interface I2 {
  toString(): boolean; // error  
  toLocaleString(): string; // valid
  valueOf(): Object; // valid
  hasOwnProperty(): boolean; // error
  isPrototypeOf(v: string): boolean; // error
  propertyIsEnumerable(v: PropertyKey): string; // error
}

interface I3 {
  toString(): string; // valid
  valueOf(): number; // error
}

interface I4 {
  valueOf(): number; // error
  toString(): string; // valid
}