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

interface Value1<T>{
    Value1Field:T
}
interface Value2{
    Value2Field:Value1<String>
}

function main(){
    let subValue1:Value1<String> = {Value1Field:"Me"}
    let subValue2:Value2 = {Value2Field:subValue1}
    let subValue3:Value2 = {Value2Field:{Value1Field:"Me"}}
}