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

void 0;

void 'hello';

void (1 + 2);

void { a: 1, b: 2 };

void [1, 2, 3];

void console.log('expression evaluated');

const undefined_value = void 1;

const undefined_value2: undefined = void 2;

const undefined_value3: number | undefined = void 3;

void void 1;

void function () {
  console.log('foo');
};

void function () {
  console.log("bar!");
}();

void (function () {
  console.log('baz!');
})();

void class {};

void (class {});

void (() => {});

function foo() {
  let a = 1;
  void a++;

  let b = [1, 2, 3];
  void console.log(b.filter(x => x % 2 !== 0));

  void function() {
    console.log('foo');
  };

  void function localFun() {
    console.log('foo');
  };

  void class {};

  void class localClass{};
}

function bar(value: string | undefined) {
    if (value === void(0)) {
        return ''
    }
}
