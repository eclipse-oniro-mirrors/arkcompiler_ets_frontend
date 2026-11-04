/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

type RecordData = undefined | Record<string, RecordData> | Array<RecordData> | Object
type RecordType =  Record<string, RecordData>
class Test {}

let want: RecordType = {
  "keyRecord": {
    "subKeyStr" : 11
  },
  "keyString": "string***&&&",
  "keyBool": true,
  "keyInt": 11,
  "keyDouble": 3.1415,
  "keyObj": new Test(),

  "arrString": ["1s", "2s", "3s"],
  "arrBool": [true, false, true, true],
  "arrInt": [1, 2, 3],
  "arrDouble": [1.1, 1.2, 1.3],
  "arrObj": [
      new Test(), 

      {
        "keyString": "string***&&&",
        "keyBool": true,
        "keyInt": 11,
        "keyDouble": 3.1415,
        "keyObj": new Test()
      },

      {
          "arrString": ["1s", "2s", "3s"],
          "arrBool": [true, false, true, true],
          "arrInt": [1, 2, 3],
          "arrDouble": [1.1, 1.2, 1.3],
      },
    ],

  "keyRecord2" : {
    "keyString": "string",
    "arrString": ["1s", "2s", "3s"],
    "arrBool": [true, false, true, true],
    "arrInt": [1, 2, 3],
    "arrDouble": [1.1, 1.2, 1.3],
    "innerArrObj": [
      new Test(), 

      {
        "keyString": "string***&&&",
        "keyBool": true,
        "keyInt": 11,
        "keyDouble": 3.1415,
        "keyObj": new Test()
      },

      {
          "arrString": ["1s", "2s", "3s"],
          "arrBool": [true, false, true, true],
          "arrInt": [1, 2, 3],
          "arrDouble": [1.1, 1.2, 1.3],
      },
    ],
  },

  "arrArray" : [ 
    ["1s", "2s", "3s"],
    [true, false, true, true],
    [1, 2, 3],
    [1.1, 1.2, 1.3],
    [
      {
        "arrString": ["1s", "2s", "3s"], 
        "arrBool": [true, false, true, true],
        "arrInt": [1, 2, 3],
        "arrDouble": [1.1, 1.2, 1.3],
        "keyRecord": { "subKeyStr" : 11 }
      }, 

      {
        "arrString": ["1s", "2s", "3s"], 
        "arrBool": [true, false, true, true],
        "arrInt": [1, 2, 3],
        "arrDouble": [1.1, 1.2, 1.3],
        "keyRecord": { "subKeyStr" : 11 }
      }
    ]
  ]
}
