/**
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http: //www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

interface I {
    i:int;
    j:string;
}

interface I2 {
    readonly i:int;
    j:string;
}

function main(){
    let partial1:Partial<I> = {i:1234,j:"abcd"};
    let partial2:Partial<I> = {a:1234};
    let partial3:Partial<I> = {i:"abcd"};

    let partial4:Partial<I2> = {i:1234,j:"abcd"};
    partial4.i = 100;
}


/* @@? 28:32 Error Semantic error ESE0065: type I$partial has no property named a */
/* @@? 29:34 Error Semantic error ESE0319: Type '"abcd"' is not compatible with type 'Int|undefined' at property 'i' */
/* @@? 32:14 Error Semantic error ESE0209: Cannot assign to this property because it is readonly. */