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

class UndefinedAssignment {
  UndefinedAssignment() {
  }

  static undefined1() {
    // Direct assignment of 'undefined'
    let x: undefined = undefined;
    return x;
  }

  static undefined2() {
    let z: number = 5;
    return z;
  }
}

class StandardFunctionReturns {
  StandardFunctionReturns() {
  }

  static checkSideEffect() {
    let temp = 'executed';
  }

  static standard1() : void {
    // A simple function that implicitly returns undefined
    return StandardFunctionReturns.checkSideEffect();
  }

  static standard2() {
    if (1 > 0) {
      return;
    }
  }
}

class NonVoidConditional {
  NonVoidConditional() {
  }

  static cond1() {
    // Ternary operator returning 'undefined'
    let x: number = -1;
    return 1 < 0 ? 1 : undefined;
  }

  static cond2() {
    let y: number = 0;
    if (y === 1) {
      return y;
    }
    return y;
  }
}