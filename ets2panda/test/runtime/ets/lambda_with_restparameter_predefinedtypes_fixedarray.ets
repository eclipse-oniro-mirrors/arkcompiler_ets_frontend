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

let f1 = (a:number, b:number, ...args:FixedArray<number>)=>{
    arktest.assertEQ(a,0);
    arktest.assertEQ(args[0],222);
}

function main(){
    let f2 = (a:number, b:number, ...args:FixedArray<number>)=>{
        arktest.assertEQ(a,0);
        arktest.assertEQ(args[0],222);
    }
    f1(0,111,222,333)
    f2(0,111,222,333)
}
