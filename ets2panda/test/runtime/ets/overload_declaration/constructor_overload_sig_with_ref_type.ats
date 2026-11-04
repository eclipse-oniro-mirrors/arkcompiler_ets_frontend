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
    a:Int = 1
    constructor constructor1(){}
    constructor constructor2(a:Int){}
    overload constructor{ constructor1, constructor2 }
}

class B{
    a:Number = 1
    constructor(){}
    constructor constructor1(a:Number){}
    overload constructor{ constructor1 }
}

class C{
    a:String = "1"
    constructor constructor1(){}
    constructor constructor2(a:String){}
    overload constructor{ constructor1, constructor2 }
}

function main() : void {
    let a = new A(5);
    let b = new B(5);
    let c = new C("5");
}
