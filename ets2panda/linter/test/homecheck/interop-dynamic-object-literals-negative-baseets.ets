/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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


import { A } from './fixtures/homecheck-interop-object-literals/ExportTs'
import { B } from './fixtures/homecheck-interop-object-literals/ExportEts1'

function test1() {
  let obj: A = { name: 'xyz'};
  let _ = obj instanceof A;     // 告警
}

function test2() {
  let obj: B = {v: 2};
  let obj2 = obj;
  let _ = obj2 instanceof B;     // 告警
}

class ParamCheck {
  test3() {
    let obj: A = { name: 'xyz'};
    this.test4(obj);
  }

  test4(obj: A) {
    let _ = obj instanceof A;     // 告警
  }
}

class ConditionCheck1 {
  test3() {
    let obj: A = { name: 'xyz'};
    this.test4(obj);
  }

  test4(obj: A) {
    if (obj instanceof A) {     // 告警

    }
  }
}

class ConditionCheck2 {
  test3() {
    let obj: B = {v: 2};
    this.test4(obj, false);
  }

  test4(obj: B, cond: boolean) {
    if (obj instanceof B && cond) {     // 告警

    }
  }
}

class ReturnValueCheck {
  test3() {
    let obj: B = {v: 2};
    return obj;
  }

  test4() {
    let obj2 = this.test3();
    let _ = obj2 instanceof B;     // 告警
  }
}

class TryCatchFinallyCheck {
  test() {
    let codeATry: B = {v: 1};
    let codeACatch: B = {v: 'catch'};
    let codeAFinally: B = {v: 'finally'};
    try {
      console.info('testTag', '' + (codeATry instanceof A));     // 告警
    } catch (e) {
      console.info('testTag', '' + (codeACatch instanceof A));     // 告警
    } finally {
      console.info('testTag', '' + (codeAFinally instanceof A));     // 告警
    }
  }
}

class NoCheckAll {
  test1(): A {
    const a: A = {name: 'test1'};
    return a;
  }

  test2(): A {
    return this.test1();
  }

  test3(): A {
    return this.test2();
  }

  test4() {
    if (this.test3() instanceof A) { }     // 告警
  }

  test5(param5: B) {
    if (param5 instanceof A) { }     // 告警
  }

  test6(param6: B) {
    this.test5(param6);
  }

  test7(param7: B) {
    this.test6(param7);
  }

  test8() {
    let b: B = {v: 1};
    this.test7(b);
  }
}