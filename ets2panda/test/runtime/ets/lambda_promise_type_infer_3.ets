/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

let al1  = async (a: boolean) => {
    if (a) {
        return 1
    } else {
        return Promise.resolve(1)
    }
}
let l1  = (a: boolean) => {
    if (a) {
        return 1
    } else {
        return Promise.resolve(1)
    }
}

let al2  = async (a: boolean) => {
    if (a) {
        return 1
    } else {
        return Promise.resolve('s')
    }
}
let l2  = (a: boolean) => {
    if (a) {
        return 1
    } else {
        return Promise.resolve('s')
    }
}

let al3 = async (a: boolean) => {
    if (a) {
        return Promise.resolve('s')
    } else {
        return 1
    }
}

let l3 = (a: boolean) => {
    if (a) {
        return Promise.resolve('s')
    } else {
        return 1
    }
}

// check no regression for #33499
async function apair() : Promise<[number, string]> {
    return [1.0, 'hello']
}

async function main() {
    arktest.assertTrue(al1(true) instanceof Promise);
    arktest.assertEQ(await al1(true), 1);
    arktest.assertTrue(al1(false) instanceof Promise);
    arktest.assertEQ(await al1(false), 1);
    let al1rest : Promise<Int> = al1(true)
    arktest.assertEQ(await al1rest, 1);
    let al1resf : Promise<Int> = al1(false)
    arktest.assertEQ(await al1resf, 1);

    arktest.assertTrue(l1(true) instanceof Int);
    arktest.assertEQ(l1(true) as Int, 1);
    arktest.assertTrue(l1(false) instanceof Promise);
    arktest.assertEQ(await (l1(false) as Promise<Int>), 1);
    let l1rest : Int | Promise<Int> = l1(true)
    arktest.assertEQ(l1rest as Int, 1);
    let l1resf : Int | Promise<Int> = l1(false)
    arktest.assertEQ(await l1resf, 1);

    arktest.assertTrue(al2(true) instanceof Promise);
    arktest.assertEQ(await al2(true), 1);
    arktest.assertTrue(al2(false) instanceof Promise);
    arktest.assertEQ(await al2(false), 's');
    let al2rest : Promise<Int | String> = al2(true)
    arktest.assertEQ(await al2rest, 1);
    let al2resf : Promise<Int | String> = al2(false)
    arktest.assertEQ(await al2resf, 's');

    arktest.assertTrue(l2(true) instanceof Int);
    arktest.assertEQ(l2(true) as Int, 1);
    arktest.assertTrue(l2(false) instanceof Promise);
    arktest.assertEQ(await (l2(false) as Promise<String>), 's');
    let l2rest : Int | Promise<String> = l2(true)
    arktest.assertEQ(l2rest as Int, 1);
    let l2resf : Int | Promise<String> = l2(false)
    arktest.assertEQ(await l2resf, 's');

    arktest.assertTrue(al3(true) instanceof Promise);
    arktest.assertEQ(await al3(true), 's');
    arktest.assertTrue(al3(false) instanceof Promise);
    arktest.assertEQ(await al3(false), 1);
    let al3rest : Promise<Int | String> = al3(true)
    arktest.assertEQ(await al3rest, 's');
    let al3resf : Promise<Int | String> = al3(false)
    arktest.assertEQ(await al3resf, 1);

    arktest.assertTrue(l3(true) instanceof Promise);
    arktest.assertEQ(await l3(true), 's');
    arktest.assertTrue(l3(false) instanceof Int);
    arktest.assertEQ(l3(false), 1);
    let l3rest : Int | Promise<String> = l3(true)
    arktest.assertEQ(await l3rest, 's');
    let l3resf : Int | Promise<String> = l3(false)
    arktest.assertEQ(l3resf, 1);
}
