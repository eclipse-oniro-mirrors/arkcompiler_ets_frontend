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

interface OptionsA {
    menuList?: Array<string>;
}

interface OptionsB {
    menuList: FixedArray<string>;
}

function heeA(opt: OptionsA): Array<string> {
    let menuList: Array<string> = opt.menuList ?? [];
    return menuList;
}

function heeB(opt: OptionsB): FixedArray<string> {
    let menuList: FixedArray<string> = opt.menuList == undefined ? [] : opt.menuList;
    return menuList;
}

function main(): void {
    const options1: OptionsA = { menuList: ["Home", "About", "Contact"] };
    const options2: OptionsA = { menuList: [] };
    const options3: OptionsA = {};
    const options4: OptionsA = { menuList: undefined };
    arktest.assertEQ(heeA(options1).length, 3);
    arktest.assertEQ(heeA(options1)[0], "Home");
    arktest.assertEQ(heeA(options1)[1], "About");
    arktest.assertEQ(heeA(options1)[2], "Contact");
    arktest.assertEQ(heeA(options2).length, 0);
    arktest.assertEQ(heeA(options3).length, 0);
    arktest.assertEQ(heeA(options4).length, 0);

    const options5: OptionsB = { menuList: ["Home", "About", "Contact"] };
    const options6: OptionsB = { menuList: [] };
    arktest.assertEQ(heeB(options5).length, 3);
    arktest.assertEQ(heeB(options5)[0], "Home");
    arktest.assertEQ(heeB(options5)[1], "About");
    arktest.assertEQ(heeB(options5)[2], "Contact");
    arktest.assertEQ(heeB(options6).length, 0);
}
