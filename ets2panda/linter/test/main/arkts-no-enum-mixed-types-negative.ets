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

enum Numbers {
  A = 10,
  B = 2.57, // err
  C = 0x2B7F,
  D = -1.5, // err
  E = 1.1e2 // err
}

enum Direction {
  Up = 'UP',
  Down = 'DOWN',
  Left = 'LEFT',
  Right = 'RIGHT',
  Across = 'UP'+'LEFT',  // no err
}

const constVar = 25;
let letVar = 50;

enum ExprInits {
  A = constVar,
  B = letVar, // err
  C = A, // no err
  D = Direction.Right, // no err
  E = Numbers.C, // no err
  H = '123'.length, // err
  I = 1 && 2, // no err
}

enum TT1 {
  A = 1,
  B = 2
}


enum TT2 {
  A = 's',
  B = 'aa'
}

enum TT3 {
  A = TT1.A,
  B = TT2.A //err
}

enum TR2 {
  Z = 1,
  H = 2,
  M = 4.4 + 4 //err
}

enum TE1 {
  A = 1.0, // err
  B = 2
}

enum Numbers14{
  RED = 1 as number, // err
  BLUE = 2,
  GREEN = -1 // no err
}

const enum RTE {
  A = 'aaa',
  B = 2 // err
}
