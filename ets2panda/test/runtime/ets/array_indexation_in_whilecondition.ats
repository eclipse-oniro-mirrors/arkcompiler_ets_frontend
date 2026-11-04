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

function main():void{
    let x1:Int = 1;
    while(([false,true])[x1]){
        break;
    }
    arktest.assertTrue(([false,true])[x1])

    let x2:Int = 1;
    let i = 4;
    while(([false,true])[x2] && i > 0){
        --i;
    }
    arktest.assertTrue(([false,true])[x2])

    let x3 = [false,true]
    while(x3[1]){
        break;
    }
    arktest.assertTrue(x3[1])

    let x4 = [true,false]
    while(!x4[1]){
        break;
    }
    arktest.assertTrue(!x4[1])
}