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

 
interface B {
    readonly name: string
}

interface A {
    readonly age:number
    readonly Name: B
}
function main() {
    let a:A = {age: 20, Name:{name:"John"}}
    a.Name.name = "John2"
}


/* @@? 27:12 Error Semantic error ESE0209: Cannot assign to this property because it is readonly. */