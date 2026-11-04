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

function foo(c1: (a: string)=>void, c2: ()=>void) {}

function foo(c2: (a: int)=>void) {

}

function main() {
    // signature with trailling lambda will have high precedence, thus the following will match the first signature,
    // expected to throw error because the first parameter is not compatible.
    foo((a: int)=>{})
    {
        console.println("call back!")
    }
}
