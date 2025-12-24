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

void 0;       // No void operator

void 'hello';       // No void operator

void (1 + 2);       // No void operator

void [1, 2, 3];       // No void operator

void console.log('expression evaluated');       // No void operator

const undefined_value = void 1;       // No void operator

const undefined_value2: undefined = void 2;       // No void operator

const undefined_value3: number | undefined = void 3;       // No void operator

void void 1;       // No void operator

void (() => {});       // No void operator

function foo() {
  let a = 1;
  void a++;       // No void operator

  let b = [1, 2, 3];
  void console.log(b.filter(x => x % 2 !== 0).toString());       // No void operator
}

function getEmpty() {
  return;
}

function undefined() {
  let y = getEmpty();       // Can't use void as value
  return y;
}

class B{
  s15 = void 'hello';//(arkts-no-void-operator)
  public s16 = void 1;//(arkts-no-void-operator)
  private s17 = void 1;//(arkts-no-void-operator)
  protected s18 = void 1;//(arkts-no-void-operator)
  static s19 = void 1;//(arkts-no-void-operator)
}
