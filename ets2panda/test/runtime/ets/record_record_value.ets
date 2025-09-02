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

interface PersonInfoInterface1{
    age1:number
    salary1:number
}

interface PersonInfoInterface2{
    age2:PersonInfoInterface1
    salary2:number
}

interface PersonInfoInterface3{
    age3:PersonInfoInterface2
    salary3:number
}

function assertsalary1(inputval:PersonInfoInterface1|undefined, salary:int){
    if(inputval instanceof undefined){
        arktest.assertTrue(false)
    }
    let returnval = inputval as PersonInfoInterface1;
    arktest.assertEQ(returnval.salary1, salary)
}

function assertsalary3(inputval:PersonInfoInterface3|undefined, salary:int){
    if(inputval instanceof undefined){
        arktest.assertTrue(false)
    }
    let returnval = inputval as PersonInfoInterface3;
    arktest.assertEQ(returnval.salary3, salary)
}

function main(){
    let personMap1:Record<string, PersonInfoInterface1> = {
        "john":{age1:25, salary1:10},
        "Mary":{age1:21, salary1:20}
    };
    assertsalary1(personMap1.get("john"), 10);

    let nestedObjectMap:Record<string, PersonInfoInterface3> = {
        "john":{age3:{age2:{age1:10,salary1:10},salary2:10},salary3:10},
        "Mary":{age3:{age2:{age1:10,salary1:10},salary2:10},salary3:10}
    };
    assertsalary3(nestedObjectMap.get("john"), 10);
}