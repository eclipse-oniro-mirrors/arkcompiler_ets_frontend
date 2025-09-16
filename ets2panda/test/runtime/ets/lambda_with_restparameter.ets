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
let globalVariable:String = "globalVariable";

function main(){
    let localVariable:String = "localVariable"
    let f1 = (a:String, b:number, ...args:(number|string|undefined)[])=>{
        arktest.assertEQ(a,"000");
        arktest.assertEQ(b,111.0);
        arktest.assertEQ(args[0],123.0);
        arktest.assertEQ(localVariable,"localVariable");
        arktest.assertEQ(globalVariable,"globalVariable")
    }
    f1("000",111.0,123.0,"123")
}
