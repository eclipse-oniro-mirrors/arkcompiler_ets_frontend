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
class A_class{
    localfield:string = "localstring";
    foo(){
        abstract class AbstractLocalClass{}
        class LocalClass extends AbstractLocalClass{
            method1():string{
                return localfield;
            }
        }
        final class FinalLocalClass{
            method1():string{
                return this.localfield;
            }
        }

        class ExtendsLocalClass extends FinalLocalClass{}
        abstract final class AbstractFinalLocalClass{}

        abstract class AbstractLocalClass2{
            method1(){}
        }
        let a:AbstractLocalClass2 = new AbstractLocalClass2();

        abstract class AbstractLocalClass3{
            abstract override method1()
            abstract method2(){}
        }
        
        final class FinalLocalClass2 extends AbstractLocalClass3{
            override method2(){}
            abstract method3()
        }

        
    }
}


/* @@? 18:18 Error Syntax error ESY0040: Illegal start of CLASS expression. */
/* @@? 19:9 Error Syntax error ESY0040: Illegal start of CLASS expression. */
/* @@? 24:15 Error Syntax error ESY0040: Illegal start of CLASS expression. */
/* @@? 30:9 Error Syntax error ESY0040: Illegal start of CLASS expression. */
/* @@? 31:24 Error Syntax error ESY0040: Illegal start of CLASS expression. */
/* @@? 33:18 Error Syntax error ESY0040: Illegal start of CLASS expression. */
/* @@? 36:15 Error Semantic error ESE0371: Cannot find type 'AbstractLocalClass2'. */
/* @@? 36:41 Error Semantic error ESE0371: Cannot find type 'AbstractLocalClass2'. */
/* @@? 38:18 Error Syntax error ESY0040: Illegal start of CLASS expression. */
/* @@? 43:15 Error Syntax error ESY0040: Illegal start of CLASS expression. */