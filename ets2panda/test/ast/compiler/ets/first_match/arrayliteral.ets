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

class Test{
    foo1(a:[string,string,string]){
        console.log("invoke1")
    }
    foo2(a:[number,string,boolean]){
        console.log("invoke2")
    }
    foo3(a:number[]){
        console.log("invoke3")
    }

    overload foo111{foo1,foo2,foo3}
}

function main(){
    let a:Test = new Test();
    a.foo111(["abc","abc",1]);
    a.foo111(["abc","abc",true]);
    a.foo111([123,"abc",true]);
    a.foo111([123,123,123]);
}

/* @@? 32:5 Error TypeError: No matching call signature for foo111(Array<String|Int>) */
/* @@? 33:5 Error TypeError: No matching call signature for foo111(Array<String|Boolean>) */
