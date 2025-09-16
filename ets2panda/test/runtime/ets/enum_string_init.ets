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

const b = 1 + 1 * 2
const a = b + 10
enum Color {
    Red = b + "a" + 1 + 1,
    Blue = a + "Blue" + 3.123414515123516135135,
    Yelllow = "a" + 123401841039413413413431.123124,
}

function main() {
    let redString : string = b + "a" + 1 + 1;
    arktest.assertEQ(Color.Red.toString(), redString)
    
    let blueString = a + "Blue" + 3.123414515123516135135;
    arktest.assertEQ(Color.Blue.toString(), blueString)

    let yellowString = "a" + 123401841039413413413431.123124;
    arktest.assertEQ(Color.Yelllow.toString(), yellowString)
}