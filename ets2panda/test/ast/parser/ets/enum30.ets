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
 
enum Color { Red = 100, Green, Blue }

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
    H = Color.Red ? ColorE.G : (ColorD.E | 1),
    I = (ColorE.G / 25)
}

enum ColorG {
    Red = "O",
    Green = "O"
}
enum Colorj {
    B = "abc",
    M = ColorF.I == 4 ? ColorG.Red + ColorG.Green : "ac"
}
