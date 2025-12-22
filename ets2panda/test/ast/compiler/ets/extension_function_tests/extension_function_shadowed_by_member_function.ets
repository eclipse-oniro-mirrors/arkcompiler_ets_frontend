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
    xxx() {
    }

    static sweetness() {
    }

    price(id: int): int {
        return 1;
    }
}

class B extends A {
    price(): int {
        return 1;
    }

    sweetness1(id: String) {
    }
}

function sweetness(this:A) {
}

function sweetness1(this:B, id1: String) /* @@label1 */{
}

function price(this:B) /* @@label2 */{
}

function xxx(this:A, id: int) {
}


/* @@@ label1 Error Semantic error ESE0081: The extension function 'sweetness1' has the same name with public method in class B */
/* @@@ label2 Error Semantic error ESE0081: The extension function 'price' has the same name with public method in class B */