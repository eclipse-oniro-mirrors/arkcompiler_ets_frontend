/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

(() => {
    0;
    return undefined;
})();       // No void operator

(() => {
    'hello';
    return undefined;
})();       // No void operator

(() => {
    (1 + 2);
    return undefined;
})();       // No void operator

(() => {
    [1, 2, 3];
    return undefined;
})();       // No void operator

(() => {
    console.log('expression evaluated');
    return undefined;
})();       // No void operator

const undefined_value = (() => {
    1;
    return undefined;
})();       // No void operator

const undefined_value2: undefined = (() => {
    2;
    return undefined;
})();       // No void operator

const undefined_value3: number | undefined = (() => {
    3;
    return undefined;
})();       // No void operator

(() => {
    void 1;
    return undefined;
})();       // No void operator

(() => {
    (() => { });
    return undefined;
})();       // No void operator

function foo() {
  let a = 1;
  (() => {
    a++;
    return undefined;
  })();       // No void operator

  let b = [1, 2, 3];
  (() => {
    console.log(b.filter(x => x % 2 !== 0).toString());
    return undefined;
  })();       // No void operator
}

function getEmpty() {
  return;
}

function undefined() {
  let y = getEmpty();       // Can't use void as value
  return y;
}

class B{
  s15 = (() => {
    'hello';
    return undefined;
  })();//(arkts-no-void-operator)
  public s16 = (() => {
    1;
    return undefined;
  })();//(arkts-no-void-operator)
  private s17 = (() => {
    1;
    return undefined;
  })();//(arkts-no-void-operator)
  protected s18 = (() => {
    1;
    return undefined;
  })();//(arkts-no-void-operator)
  static s19 = (() => {
    1;
    return undefined;
  })();//(arkts-no-void-operator)
}
