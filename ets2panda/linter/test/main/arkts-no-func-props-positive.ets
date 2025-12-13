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

class ObjectDefinitions {
  ObjectDefinitions() {
  }

  static definition1() {
    // Only primitives.
    const id: number = 5;
    const offset: number = 10;

    // Accessing a property.
    return id + offset;
  }

  static definition2() {
    // Array of strings.
    const calculateValues = ['a', 'b', 'c'];
    // Returning array length.
    return calculateValues.length;
  }
}

class ConstantUsage {
  ConstantUsage() {
  }

  static usage1() {
    // Simple constant assignment.
    const message: string = "Complete";
    let result = message;
    return result;
  }

  static usage2() {
    // Defining a simple data object.
    const status: number = 200;
    // Returning a property value.
    return status;
  }
}

class SimpleExpressions {
  SimpleExpressions() {
  }

  static scope1() {
    let multiplier = 2;
    // Simple arithmetic expression.
    let input: number = 7;
    return input * multiplier;
  }

  static scope2() {
    let active = true;
    let count = 5;
    // Logical expression.
    if (active && count > 0) {
      return 'Valid';
    }
    return 'Invalid';
  }
}

function foo() {}

console.log(foo.name)
