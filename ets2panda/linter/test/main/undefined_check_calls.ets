/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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
import { ff, doSomething, TestClassC, TestClassD, TestClassH, TestClassI } from "./oh_modules/ohos_lib";
import { ff as ff_ets, doSomething as doSomething_ets, TestClassC as TestClassC_ets, TestClassD as TestClassD_ets, TestClassH as TestClassH_ets, TestClassI as TestClassI_ets } from "./oh_modules/ohos_lib_ets";
import { f, f2, bar, applyToUnknown, fooExecute, C1, resourceFoo, ResourceStr, myFoo, myFoo2 } from "./dynamic_lib";

let a1: C1 = new C1()

function g1(): C1 | undefined {
    if (a1) {
        return a1
    }
    return undefined
}

f2(g1())

bar(undefined);
bar(undefined, undefined);
applyToUnknown(undefined);
fooExecute(undefined);

function fff(a: Array<number>): void {}

fff(undefined);
ff(undefined);
f(undefined);

function resourceFoo2(a: ResourceStr) { return "0" }

class A {
    buttonTitle1?: string;
    buttonTitle2?: string;
    buttonTitle3?: string;

    constructor(menu?: Partial<A>) {
        this.buttonTitle1 = resourceFoo(menu?.buttonTitle1);
        this.buttonTitle2 = resourceFoo(menu?.buttonTitle2);
        this.buttonTitle3 = resourceFoo2(menu?.buttonTitle3);
    }
}

myFoo({x: "", y: undefined})

class A2 {
    public a: Object = undefined
    f() {
        myFoo2(
            () => {
                this.a = undefined;
            }
        )
    }
}

class TestClassB {
  private c:TestClassD | undefined = new TestClassD();
  add() {
    doSomething({
      topic: (topic: string) => {
        let a = new TestClassC();
        this.c = a;
        this.c = a;
      }
    }, '');
  }
}

ff_ets(undefined);
new TestClassH(undefined);
new TestClassH_ets(undefined);

class TestClassE {
  private c:TestClassD_ets | undefined = new TestClassD_ets();
  add() {
    doSomething_ets({
      topic: (topic: string) => {
        let a = new TestClassC_ets();
        this.c = a;
        this.c = a;
      }
    }, '');
    new TestClassI({
      topic: (topic: string) => {
        let a = new TestClassC();
        this.c = a;
        this.c = a;
      }
    }, '');
    new TestClassI_ets({
      topic: (topic: string) => {
        let a = new TestClassC_ets();
        this.c = a;
        this.c = a;
      }
    }, '');
  }
}