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
 
enum Color { Red, Green, Blue }

enum ColorB {
    A = Color.Red,
    B = Color.Green
}
enum ColorC {
    C = Color.Green,
    D
}
enum ColorD {
    E = Color.Red,
    F = 1
}
enum ColorE {
    G = Color.Red | 1
}
enum ColorF {
    H = Color.Red | 1,
    I
}
enum ColorG {
    Red = "O",
    Green = "O"
}
enum ColorH {
    J = ColorG.Green,
    K = "O"
}
enum ColorI {
    L = ColorH.J
}
enum Colorj {
    M = Color.Red + Color.Green
}

class A {
    Red: number = 1
}
const AInstance = new A();
enum ColorA {
    A = /* @@ label */AInstance.Red
}

function main(): void {
  const red = Color.Red;
  switch (Color.Green) {
    case Color.Blue: break;
    case /* @@ label1 */red: break;
    case Color.Green: break;
  }
}


/* @@@ label Error Semantic error ESE0342: Enumeration members can be initialized only by compile-time expressions and initializers must be of the same type. */
/* @@@ label1 Error Semantic error ESE0334: Enum switch case must be unqualified name of an enum constant */