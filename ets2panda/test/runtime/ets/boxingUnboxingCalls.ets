/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

function intParam(a: int): int {
    return 0;
}

function objParam(a: Object): int {
    return 2;
}

function retRefInt(a: int): Int {
    return new Int(a);
}

function retInt(a: Int): int {
    return a;
}

function main(): void {
    let a: int = 1;
    let b: Int = 2;
    let c: Int = intParam(a);
    arktest.assertEQ(c, 0)
    let f: Int = intParam(b);
    arktest.assertEQ(f, 0)
    let g: int = objParam(a);
    arktest.assertEQ(g, 2)
    let h: int = objParam(b);
    arktest.assertEQ(h, 2)
    let i: Object = b;
    let j: int = retInt(retRefInt(5));
    arktest.assertEQ(j, 5)
}
