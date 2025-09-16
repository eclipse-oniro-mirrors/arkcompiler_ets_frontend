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

class C{
    out: number = 0;
    in: number = 0;
    data: number = 0;
}

function process1(con: (this:C)=>void){
    let c = new C();
    c.con();
    arktest.assertEQ(c.out, 1);
    arktest.assertEQ(c.in, 2);
    arktest.assertEQ(c.data, 3);
}

function process2(con: (this:C)=>void){
    let c = new C();
    c.con();
    arktest.assertEQ(c.out, 1);
    arktest.assertEQ(c.in, 2);
    arktest.assertEQ(c.data, 3);
}

let field_out:number = 1;
function main(){
    let field_in:number = 2;
    // passed lambda as parameter.
    process1((this:C):void => {
        this.out = field_out;
        this.in = field_in;
        this.data = 3;
    })

    // passed lambda as trailing lambda.
    process2() {
        this.out = field_out;
        this.in = field_in;
        this.data = 3;
    }
}