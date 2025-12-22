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

namespace MySpace {
    let privateVariable = 123;

    function foo(){}

    interface myInterface{}

    class innerClass implements myInterface{} //ok visible

    enum Color{RED, BLUE, GREEN}

    @interface Anno{
        color :Color
    }

    console.log(privateVariable) //ok visible
    new innerClass() //ok visible
    
    @Anno(Color.RED)  //ok visible
    class C{
        static a:int = 1
    }

    foo()  //ok visible
}

class A implements MySpace.myInterface{} // CTE

new MySpace.innerClass()  // CTE

@MySpace.Anno(MySpace.Color.RED) // CTE
class C{}

MySpace.privateVariable = 456; // CTE

MySpace.foo()  // CTE

new MySpace.C() // CTE

let a = new Array<MySpace.C>() // CTE

MySpace.C.a // CTE

/* @@? 42:28 Error Semantic error ESE0206: 'myInterface' is not exported in 'MySpace' */
/* @@? 44:13 Error Semantic error ESE0206: 'innerClass' is not exported in 'MySpace' */
/* @@? 46:10 Error Semantic error ESE0206: 'Anno' is not exported in 'MySpace' */
/* @@? 46:23 Error Semantic error ESE0206: 'Color' is not exported in 'MySpace' */
/* @@? 49:9 Error Semantic error ESE0206: 'privateVariable' is not exported in 'MySpace' */
/* @@? 51:9 Error Semantic error ESE0206: 'foo' is not exported in 'MySpace' */
/* @@? 53:13 Error Semantic error ESE0206: 'C' is not exported in 'MySpace' */
/* @@? 55:27 Error Semantic error ESE0206: 'C' is not exported in 'MySpace' */
/* @@? 57:9 Error Semantic error ESE0206: 'C' is not exported in 'MySpace' */