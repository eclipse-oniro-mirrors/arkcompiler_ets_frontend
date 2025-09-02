/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

enum Color {Red = 0, Green = 1, Blue = 0}
enum Commands { Open = "fopen", Close = "fclose", Open2 = "fopen" }

let ie : Color = Color.Blue;
switch (ie) {
    case Color.Red:
        arktest.assertTrue(false)
        break;
    case Color.Green:
        arktest.assertTrue(false)
        break;
    case Color.Blue:
        break;
}

let se : Commands = Commands.Close;
switch (se) {
    case Commands.Open:
        arktest.assertTrue(false)
        break;
    case Commands.Close:
        break;
    case Commands.Open2:
        arktest.assertTrue(false)
        break;
}
