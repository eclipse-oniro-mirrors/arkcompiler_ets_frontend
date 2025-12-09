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

interface Fruit {}

interface Yellow {
    foo(id0: int, k0: String): String {
        return "3";
    }
}

function foo(this:Fruit, id1: int, k1: String): int {
    return 1;
}

interface SubFruit extends Fruit {}

class Banana implements SubFruit, Yellow {}

function foo(this:Banana, id2: int, k2: String): int/* @@ label */{
    return 2;
}


/* @@@ label Error Semantic error ESE0081: The extension function 'foo' has the same name with public method in class Yellow */