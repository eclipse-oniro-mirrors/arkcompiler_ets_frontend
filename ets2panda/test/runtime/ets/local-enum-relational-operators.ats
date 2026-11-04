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

enum Color1 {Red = 0, Green = 1, Blue = 0}
enum Color2 {First = "red", Second = "green", Third = "red"}

function main():void {
    if (Color1.Red < Color1.Blue)
        arktest.assertTrue(false)
    if (Color1.Red > Color1.Blue)
        arktest.assertTrue(false)
    if (Color1.Red == Color1.Blue)
        arktest.assertTrue(true)

    if (Color2.First < Color2.Third)
        arktest.assertTrue(false)
    if (Color2.First > Color2.Third)
        arktest.assertTrue(false)
    if (Color2.First == Color2.Third)
        arktest.assertTrue(true)
}