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

enum Colors {
  Red,
  Orange,
  Yellow
}

enum Direction {
  Up = 'UP',
  Down = 'DOWN',
  Left = 'LEFT',
  Right = 'RIGHT',
  Across = 'UP'+'LEFT',  // no err
}

enum FileAccess {
  None,
  Read = 1 << 1,
  Write = 1 << 2,
  ReadWrite = Read | Write, // no err
}

enum TT5 {
  Z = 1,
  H = 2,
  M = true ? 10.0 : 20 // no err
}

enum TT1 {
  A = 1,
  B = 2
}

enum TR1 {
  A = 1,
  B = 2
}

enum TR3 {
  A = TT1.A,
  B = TR1.A // no err
}
