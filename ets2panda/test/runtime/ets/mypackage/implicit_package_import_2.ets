/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

type mytype = A;

class A {
    aMemb: number = 5;
}

class C1 {
    static stmemb: string = "c1_stmemb";
    memb: number = 2;
    bar(): number {
        return 3;
    }
    baz(): number {
        return forC1Class();
    }
}

function forC1Class(): number {
    func2(6);
    return 4;
}

let globVar1: number = 7;
