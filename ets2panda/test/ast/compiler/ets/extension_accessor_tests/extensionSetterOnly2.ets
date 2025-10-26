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

class A {
    field_: int;
}

class B {
    field_: int;
}

set field(this: A, f: int) {
    this.field_ = f;
}

get field(this: B): int {
    return this.field_;
}

new A().field++;
new A().field += 1;
++new A().field;
new A().field = 1;
new A().field = 1.23


/* @@? 32:9 Error Semantic error ESE0211: Cannot read from this property because it is writeonly. */
/* @@? 33:9 Error Semantic error ESE0211: Cannot read from this property because it is writeonly. */
/* @@? 34:11 Error Semantic error ESE0211: Cannot read from this property because it is writeonly. */
/* @@? 36:1 Error Semantic error ESE0246: Can't find the extension accessor. */