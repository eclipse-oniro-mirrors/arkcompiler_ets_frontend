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

enum Commands { Open = "fopen",  Close = "fclose" }

function main():void {
    let c: Commands = Commands.Close
    let o: Object = c // Autoboxing of enum type to its reference version
    // Such reference version type has no name, but can be detected by instanceof
    if (o instanceof Commands) {
        let d = o as Commands // And explicitly converted back by 'as' conversion
        arktest.assertEQ(d, Commands.Close)
    }

    let u : Object | Commands = Commands.Close;
    if (u instanceof Commands)
    {
        let d = o as Commands;
        arktest.assertEQ(d, Commands.Close)
    }
}