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


function main() : void {
    enum Color { Red, Green = 10, Blue }
    enum Commands { Open = "fopen",  Close = "fclose" }

    let colors = Color.values()
    assertEQ(colors[0], Color.Red)
    try {
        let red = Color.getValueOf("Red")    
        assertEQ(red, Color.Red)
    } catch (e) { assertTrue(false)}
 

    let d: Commands = Commands.Close
    assertEQ(d.valueOf(), "fclose")
    assertEQ(d.getName(), "Close")

    let c: Color = Color.Green
    assertEQ(c.valueOf(), 10)
    assertEQ(c.getName(), "Green")
}