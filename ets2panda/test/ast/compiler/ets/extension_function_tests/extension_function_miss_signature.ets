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

class Fruit {
    price: number = 1.0;
    foo() {
        console.log("public");
    }
}

function foo(this: Fruit, a:int) {
    console.log(this.price);
}


function asFuncCall() {
    let fruit = new Fruit();
    foo(fruit, 1.01, 1.1);
}
function asMethodCall() {
    let fruit = new Fruit();
    fruit.foo(1.01, 1.1);
}

function main() {
    asFuncCall();
    asMethodCall();
}


/* @@? 30:5 Error Semantic error ESE0124: Expected 2 arguments, got 3. */
/* @@? 30:5 Error Semantic error ESE0127: No matching call signature for foo(Fruit, Double, Double) */
/* @@? 34:5 Error Semantic error ESE0127: No matching call signature for foo(Double, Double) */
