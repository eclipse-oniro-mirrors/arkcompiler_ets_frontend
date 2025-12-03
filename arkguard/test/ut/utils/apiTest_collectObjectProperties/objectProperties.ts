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

export const propertyObj = {
  name1: 'Alice',
  extra1: {
    notes1: 'additional data',
    key1: {
      key2: {
        key3: 'value',
        'key4': {
          key5: {
            key6: 'value',
            'key7': 'value'
          }
        }
      }
    },
    ['computedPropertyName']: 'computedPropertyName'
  }
};

export const stringPropertyObj = {
  'name2': 'Alice',
  'extra2': {
    'notes2': 'additional data',
    'key11': {
      'key12': {
        'key13': 'value'
      }
    }
  }
};

// TypeLiteral
export interface TypeLiteralDemo {
  typeLiteral1: {
    typeLiteral2: number
  },
  typeLiteral3: string
}

// ComputendPropertyNames
export const computedPropertyObj = {
  [Symbol.iterator]: 1,
  ["dynamic" + "Property"]: 2,
  ['computedProperty']: 3,
  normalProperty: 3,
  "stringPropertyName": 4
}

// shortandPropertyAssignment
const shortandPropertyAssignmentNumber = 2;
const shortandPropertyAssignmentString = 'shortandPropertyAssignmentName';
const shortandPropertyAssignmentBoolean = true;
const shortandPropertyAssignmentUndefined = undefined;
export let shortandPropertyAssignment = {
  shortandPropertyAssignmentNumber,
  shortandPropertyAssignmentString,
  shortandPropertyAssignmentBoolean,
  shortandPropertyAssignmentUndefined
}
