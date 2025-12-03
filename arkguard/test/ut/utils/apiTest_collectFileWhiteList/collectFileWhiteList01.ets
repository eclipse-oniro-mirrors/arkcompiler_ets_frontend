/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

export enum MyEnum {
  RED01
}

export class MyClass{
  myProp01: string;
}

export let obj01 = {
  objProp: 1
}

enum MyEnum02 {
  RED02,
  BLUE02
}

struct MyStruct {
  myStructProp: string = "hello";
}

let person = {"name": "abc"};
person["age"] = 22;

let myObjectProp = 'myObjectProp';
const MyObjectPropertyObj = {
  myObjectProp,
  myObjectProp1: 'Alice',
  myObjectProp2: {
    'myObjectProp3': 'additional data',
    'myObjectProp4': {
      myObjectProp5: {
        myObjectProp6: 'value',
        'myObjectProp7': {
          myObjectProp8: {
            myObjectProp9: 'value',
            'myObjectProp10': 'value'
          }
        }
      }
    },
    ['myObjectProp11']: 'myObjectProp11'
  }
};
