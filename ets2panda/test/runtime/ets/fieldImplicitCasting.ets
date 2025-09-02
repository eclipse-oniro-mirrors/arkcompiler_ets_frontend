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

class A1 {
    private bSource: byte = 0 as byte
    private sSource: short = 1 as short
    private iSource: int = 2 as int
    private lSource: long = 3 as long
    private fSource: float = 4 as float
    private cSource: char = 5 as char
    public blTarget: long = this.bSource
    public bfTarget: float = this.bSource
    public bdTarget: double = this.bSource
    public slTarget: long = this.sSource
    public sfTarget: float = this.sSource
    public sdTarget: double = this.sSource
    public ilTarget: long = this.iSource
    public ifTarget: float = this.iSource
    public idTarget: double = this.iSource
    public lfTarget: float = this.lSource
    public ldTarget: double = this.lSource
    public fdTarget: double = this.fSource
    public clTarget: long = this.cSource
    public cfTarget: float = this.cSource
    public cdTarget: double = this.cSource
}

function main(): void {
    let a1: A1 = new A1()
    arktest.assertEQ(a1.blTarget, 0)
    arktest.assertEQ(a1.bfTarget, 0.0)
    arktest.assertEQ(a1.bdTarget, 0.0)
    arktest.assertEQ(a1.slTarget, 1)
    arktest.assertEQ(a1.sfTarget, 1.0)
    arktest.assertEQ(a1.sdTarget, 1.0)
    arktest.assertEQ(a1.ilTarget, 2)
    arktest.assertEQ(a1.ifTarget, 2.0)
    arktest.assertEQ(a1.idTarget, 2)
    arktest.assertEQ(a1.lfTarget, 3.0)
    arktest.assertEQ(a1.ldTarget, 3.0)
    arktest.assertEQ(a1.fdTarget, 4.0)
    arktest.assertEQ(a1.clTarget, 5)
    arktest.assertEQ(a1.cfTarget, 5.0)
    arktest.assertEQ(a1.cdTarget, 5.0)
}
