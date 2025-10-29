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

// Example of accessing non-exported members in nested namespaces
namespace OuterSpace {
    namespace InnerSpace {
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
       class C{}
   }

   let value = InnerSpace.privateVariable;
}

class A implements OuterSpace.InnerSpace.myInterface{} // CTE

new OuterSpace.InnerSpace.innerClass()  // CTE

@OuterSpace.InnerSpace.Anno(OuterSpace.InnerSpace.Color.RED) // CTE
class C{}

OuterSpace.InnerSpace.privateVariable = 456; // CTE


/* @@? 40:27 Error Semantic error ESE0206: 'privateVariable' is not exported in 'InnerSpace' */
/* @@? 43:31 Error Semantic error ESE0206: 'InnerSpace' is not exported in 'OuterSpace' */
/* @@? 43:42 Error Semantic error ESE0206: 'myInterface' is not exported in 'InnerSpace' */
/* @@? 45:16 Error Semantic error ESE0206: 'InnerSpace' is not exported in 'OuterSpace' */
/* @@? 45:27 Error Semantic error ESE0206: 'innerClass' is not exported in 'InnerSpace' */
/* @@? 47:13 Error Semantic error ESE0206: 'InnerSpace' is not exported in 'OuterSpace' */
/* @@? 47:24 Error Semantic error ESE0206: 'Anno' is not exported in 'InnerSpace' */
/* @@? 47:40 Error Semantic error ESE0206: 'InnerSpace' is not exported in 'OuterSpace' */
/* @@? 47:51 Error Semantic error ESE0206: 'Color' is not exported in 'InnerSpace' */
/* @@? 50:12 Error Semantic error ESE0206: 'InnerSpace' is not exported in 'OuterSpace' */
/* @@? 50:23 Error Semantic error ESE0206: 'privateVariable' is not exported in 'InnerSpace' */