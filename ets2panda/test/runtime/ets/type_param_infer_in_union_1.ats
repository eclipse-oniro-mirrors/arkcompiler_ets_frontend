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

class A<T>{}
function foo1<U>(array:Iterable<A<U>|U>){
    return "foo1 should be ok"
}
function foo2<U>(array:Iterable<U|A<U>>){
    return "foo2 should be ok too"
}

function main(){
    let a = [1,2,3]
    arktest.assertEQ(foo1(a), "foo1 should be ok")
    arktest.assertEQ(foo2(a), "foo2 should be ok too")
}