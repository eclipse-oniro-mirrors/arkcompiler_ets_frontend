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

function main() {
    let b: Object[] = [new Object()];
    b[0] = 2.0;
    let myvar: double = b[0] as double;

    arktest.assertEQ(myvar, 2.0)

    let c: Object;
    c = 2.0;
    c as double;

    let k = 2;
    if(2 == k){
        c = new Object();
    } else {
        c = new Double();
    }

    try{
        c as double;
    } catch (e) {
        if (e instanceof ClassCastError) {
        } else {
            arktest.assertEQ(1, 0)
        }
    }
}
