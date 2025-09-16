/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

class B {
    public kkkkkk: int = 7;
}

function xxx(this:B, m: int, n:int):int {
    let bb = new B();
    bb.kkkkkk = 8;
    this.kkkkkk = 9;
    let bbbbbbbbbb:(x: int, y:int) => int = (x: int, y:int): int => { return x + this.kkkkkk + m + n + bb.kkkkkk }
    return bbbbbbbbbb(2, 3);
}

function asFuncCall() {
    let b = new B();
    let res: int = xxx(b, 5, 8);
    arktest.assertEQ(res, 32 )
}

function asMethodCall() {
    let b = new B();
    let res: int = b.xxx(5, 8);
    arktest.assertEQ(res, 32 )
}

function main() {
    asFuncCall();
    asMethodCall();
}
