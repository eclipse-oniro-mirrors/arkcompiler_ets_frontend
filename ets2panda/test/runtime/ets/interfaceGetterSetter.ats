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

interface inter {
    get alma (): int;
    set alma(alma: int);

    korte: Object;

    readonly labda: double;

    foo(): int {return -1;}
    foo2(): Object {return new Object()}
    foo3(): double {return 0.0;}
}

class A implements inter{
    alma_ = 6;
    korte_ = new Object();
    labda_ = 1.0
    
    get alma (): int {return this.alma_}
    set alma(alma: int) { this.alma_ = alma}

    get korte (): Object {return this.korte_}
    set korte(korte: Object) { this.korte_ = korte}

    get labda(): double {return this.labda_;}

    override foo(): int {return this.alma_;}
    override foo2(): Object {return this.korte_;}
    override foo3(): double {return this.labda_;}
}

function main(){
    let a: inter = new A();
    arktest.assertEQ(a.foo(), a.alma)
    a.alma = 4;
    arktest.assertEQ(a.foo(), a.alma)


    arktest.assertTrue(a.foo2() === a.korte)
    a.korte = new Object();
    arktest.assertTrue(a.foo2() === a.korte)

    arktest.assertEQ(a.labda, a.foo3())
}
