/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

@Retention("SOURCE")
@interface Message{
   msg: string = ""
   id: int = 0
}

@Retention("SOURCE")
@interface Anno1{
}

@Retention("SOURCE")
@interface Anno2{
}

class MyClass{}
interface MyInterface{}
enum Color{Red,Green,BLUE}
type C = MyClass

// annotations for class
let myclass: (@Message MyClass)

// annotations for interface
let myInterface: (@Anno1 @Anno2 MyInterface)

// annotations for enum
let color: (@Anno1 Color)

// annotations for aliases
let c: (@Message @Anno2() @Anno1() C)

// annotations for type parameter
function foo<@Anno2 @Anno1 T> (a: @Message T){}
class A<@Anno1 T>{
   a: @Message @Anno2 T = undefined as T
   bar<@Message T>(){}
}

function main(){}