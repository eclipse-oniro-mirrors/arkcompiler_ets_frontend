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


class A{
    public age:number = 1.0;
}

class B{
    public name:string = "Joe";
    public age:number = 30;
}

class C{
    public salary:number = 100;
    public age:number = 30;
}

let a: A = {"age": 30}

let b: Record<string, number> = { age: 30}

let c: B = {"name" : "Annie", "age" : 25}

let d: Record<string, number> = {salary : 250, age: 30}

