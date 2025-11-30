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

class A {}

class G {}

class B {
  static a: A
}

class C {
  static g: G
}

const c = new C();

class D {
  static a: A | undefined = undefined
  static g: G;
}

class F {
  static a: A 
  static g: G | undefined = undefined;
}

const f = new F();

@Entry
@Component
struct Index {
  @State message: string = 'Hello World';
  public static abc:string ;

  build() {
      Text(this.message)
      .height('100%')
      .width('100%')
  }
}

// Top 0 case
class BB {}

class AA {
  static b: BB;
}

class CC {
  static count: number;
}

class DD {
  static config: { theme: string};
}

class EE {
  static name: string;
  constructor() {
    E.name = "default"
  }
}

// Basic Types
class PrimitiveTypes {
  static uninitializedString: string;
  static uninitializedNumber: number;
  static uninitializedBoolean: boolean;
}

// Array Types
class ArrayTypes {
  static uninitializedStringArray: string[];
  static uninitializedNumberArray: number[];
  static uninitializedObjectArray: { id: number }[];
  static uninitializedUnionArray: (string | number)[];
}

// Object Types
class ObjectTypes {
  static uninitializedSimpleObject: { name: string; age: number };
  static uninitializedNestedObject: {
    id: number;
    metadata: {
      createdAt: Date;
      tags: string[];
    };
  };
}

// Special Built-in Types
class BuiltInTypes {
  static uninitializedDate: Date;
  static uninitializedMap: Map<string, number>;
  static uninitializedSet: Set<string>;
  static uninitializedPromise: Promise<void>;
}

// Union and Intersection Types
class AdvancedTypes {
  static uninitializedUnion: string | number;
  static uninitializedIntersection: { name: string } & { age: number };
  static uninitializedLiteralUnion: 'success' | 'error';
}

// Optional and Nullable Types
class NullableTypes {
  static uninitializedOptional?: string;
  static uninitializedNull: null;
  static uninitializedUndefined: undefined;
}

// Generic Types
class GenericTypes<T> {
  static uninitializedGenericArray: T[];
  static uninitializedGenericValue: T;
}

// Function Types
class FunctionTypes {
  static uninitializedFunction: () => void;
  static uninitializedCallback: (result: string) => number;
}

// Complex Combined Types
class ComplexTypes {
  static uninitializedComplexArray: Array<{ id: number; data: Map<string, Set<number>> }>;
  static uninitializedRecord: Record<string, { value: number }>;
  static uninitializedTuple: [string, number, boolean];
}

// Custom Class Types
class User {
  id: number;
  name: string;
}

class CustomClassTypes {
  static uninitializedUser: User;
  static uninitializedUsers: User[];
}

// Enum Types
enum Status {
  Active,
  Inactive
}

class EnumTypes {
  static uninitializedStatus: Status;
  static uninitializedStatusArray: Status[];
}

// Never and Unknown Types
class SpecialTypes {
  static uninitializedNever: never;
  static uninitializedUnknown: unknown;
  static uninitializedAny: any;
}


class Test1 {
  static count: string | undefined;
}
