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

class A {
    price2(id: int): int {
        return 2;
    }
}

function price(this: A, id: int): int {
    return 1;
}

function main() {
    console.println(A.price(1));
}


/* @@? 27:5 Error Semantic error ESE0142: Call to `println` is ambiguous as `2` versions of `println` are available: `println(i: String): void` and `println(i: Boolean): void` */
/* @@? 27:5 Error Semantic error ESE0142: Call to `println` is ambiguous as `2` versions of `println` are available: `println(i: String): void` and `println(i: Byte): void` */
/* @@? 27:5 Error Semantic error ESE0142: Call to `println` is ambiguous as `2` versions of `println` are available: `println(i: String): void` and `println(i: Short): void` */
/* @@? 27:5 Error Semantic error ESE0142: Call to `println` is ambiguous as `2` versions of `println` are available: `println(i: String): void` and `println(i: Char): void` */
/* @@? 27:5 Error Semantic error ESE0142: Call to `println` is ambiguous as `2` versions of `println` are available: `println(i: String): void` and `println(i: Int): void` */
/* @@? 27:5 Error Semantic error ESE0142: Call to `println` is ambiguous as `2` versions of `println` are available: `println(i: String): void` and `println(i: Long): void` */
/* @@? 27:5 Error Semantic error ESE0142: Call to `println` is ambiguous as `2` versions of `println` are available: `println(i: String): void` and `println(i: Float): void` */
/* @@? 27:5 Error Semantic error ESE0142: Call to `println` is ambiguous as `2` versions of `println` are available: `println(i: String): void` and `println(i: Double): void` */
/* @@? 27:23 Error Semantic error ESE0087: Property 'price' does not exist on type 'A' */