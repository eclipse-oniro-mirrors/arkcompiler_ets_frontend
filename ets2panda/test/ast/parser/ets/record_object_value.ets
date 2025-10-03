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

interface PersonInfoInterface{
    age:number
    salary:number
}

class PersonInfoClass{
    age:number
    salary:number
}

function main(){
    let map:Record<string, PersonInfoInterface> = {
        "john":{age:25, salary:10},
        "Mary":{age:21, salary:20}
    };

    let map2:Record<string, PersonInfoClass> = {
        "john":{age:25, salary:10},
        "Mary":{age:21, salary:20}
    };

    let person1:PersonInfoInterface = {age:15, salary:20};
    let person2:PersonInfoInterface = {age:15, salary:20};

    let map3:Record<string, PersonInfoInterface> = {
        "john":person1,
        "Mary":person2
    };

    let errormap:Record<string, PersonInfoInterface> = {
        "john":{age:/* @@ label */"10", salary:10},
        "Mary":{age:21, salary:/* @@ label1 */"100"}
    };

    let errormap2:Record<string, PersonInfoInterface> = {
        "john":/* @@ label2 */{/* @@ label3 */agee:10, salary:10},
        "Mary":{age:21, salary:10, /* @@ label4 */other:10}
    };

    let stringarraymap:Record<string, string[]> = {
        "john":["10"],
        "Mary":["20", "30"]
    };
}
/* @@@ label Error Semantic error ESE0319: Type '"10"' is not compatible with type 'Double' at property 'age' */
/* @@@ label1 Error Semantic error ESE0319: Type '"100"' is not compatible with type 'Double' at property 'salary' */
/* @@@ label2 Error Semantic error ESE0400: Non-optional property 'salary' in type 'PersonInfoInterface' is missing in object literal. */
/* @@@ label2 Error Semantic error ESE0400: Non-optional property 'age' in type 'PersonInfoInterface' is missing in object literal. */
/* @@@ label3 Error Semantic error ESE0065: type PersonInfoInterface has no property named agee */
/* @@@ label4 Error Semantic error ESE0065: type PersonInfoInterface has no property named other */