/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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


const num = 121;

const num2 = 11 * 11;

function getSeq(num: number) {
  return num * num;
}

switch (num) {
  case 1:
    console.log('First match');
    break;
  case 1:
    console.log('First match');
    break;
  case num2:
    console.log('Second match');
    break;
  case getSeq(11):
    console.log('Third match');
    break;
  default:
    console.log('No match');
}

const str = 'abc';

switch (abc) {
  case 'abc':
    console.log('abc match');
    break;
  case 'bcd':
    console.log('bcd match');
    break;
  case 'bcd':
    console.log('bcd match');
    break;
  case 'abc'
    console.log('abc match');
    break;
  default:
    console.log('No match');
}

enum Test{
  TEST1,TEST2,TEST3
}

const value = Test.TEST1;

switch (value) {
  case Test.TEST1:
    console.log('TEST1 match');
    break;
  case Test.TEST2:
    console.log('TEST2 match');
    break;
  case Test.TEST3:
    console.log('TEST3 match');
    break;
  case Test.TEST1:
    console.log('TEST1 match');
    break;
  default:
    console.log('No match');
}

