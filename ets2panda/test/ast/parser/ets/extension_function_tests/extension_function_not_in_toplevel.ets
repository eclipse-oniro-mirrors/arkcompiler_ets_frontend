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

class Fruit {
}

{
    function price(this: Fruit) {
        console.println("this is price extension function");
    }
}

function main() {
    let fruit = new Fruit();
    fruit.price();
}


/* @@? 20:5 Error Syntax error ESY0135: Nested functions are not allowed. */
/* @@? 20:20 Error Syntax error ESY0240: Unexpected 'this' keyword in non-receiver context. */
/* @@? 27:11 Error Semantic error ESE0087: Property 'price' does not exist on type 'Fruit' */