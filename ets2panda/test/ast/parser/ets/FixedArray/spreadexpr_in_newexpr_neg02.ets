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

class A {
    fld: FixedArray<number>;
    constructor/* @@ label */(x: int, y?:number, ...p: FixedArray<number>) {
        this.fld = p;
    }
}

let nums: FixedArray<number> = [1.0, 2.0, 3.0];

function main() {
    let a2 = new A(1, 1.0, ...nums);
}


/* @@@ label Error Syntax error ESY0015: Both optional and rest parameters are not allowed in function's parameter list. */