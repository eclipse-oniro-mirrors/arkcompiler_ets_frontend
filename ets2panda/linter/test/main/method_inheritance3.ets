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
 
class A<T>{
  getData(a:T):T{
    return a;
  }
}

class B extends A<number>{
  getData(a: number): number {  // no error
    return 123;
  }
}

enum E{
  E1,
  E2
}

interface I {
  fun(e:E):void
}

class C implements I {
  fun(e:E):void{  // no error

  }
}

Class UIAbility {
  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void;
  onWindowStageCreate(windowStage: window.WindowStage): void;
}

class EntryAbility extends UIAbility {
  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void { // no error

  }

  onWindowStageCreate(windowStage: window.WindowStage): void {  // no error
    try {
      windowStage.loadContent('pages/Index', (err: BusinessError<void> | null): void => {
        if (err && err.code) {
          return;
        }
      });
    } catch (e) {
    }
  }
}

class TestA {
  a: number = 0
}

class TestD extends TestA{
  a: number = 0;
  b: number = 1;
}

class TestE implements TestI{

}

class TestF extends TestE{

}
interface TestI{

}

interface TestI2 extends TestI{

}
class TestBase {
  foo3(obj: TestD): void {
    console.log("Derived:" + obj.b)
  }

  foo4(obj: TestD): TestA {
    console.log("base" );
    return new TestA()
  }

  foo5():TestI{
    return {}
  }

  foo12(obj:never){

  }
}

class TestDerived extends TestBase {
  override foo3(obj: TestA): void {  // no error
    console.log("base" );
  }

  override foo4(obj: TestA): TestD {  // no error
    return new TestD();
  }

  override foo5(): TestI2 {  // no error
    return {}
  }

  override foo12(obj:TestA){   // no error

  }
}

class CC1 {}
class CC2 extends CC1 {
  public static toString(result: number): string { // no error
    return '';
  }
}

class AA<T>{
  getData(a:T):T{

    return a;
  }
}

class BB extends AA<number>{
  getData(a: number): number {  // no error
    return 123;
  }
}