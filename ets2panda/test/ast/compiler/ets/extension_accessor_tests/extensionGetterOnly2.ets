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

set field(this: B, n: int) {
    this.field_ = n;
}

get field(this: A): int {
    return this.field_;
}
new A()./* @@ label1 */field = 1;
new A()./* @@ label2 */field += 1;
new A()./* @@ label3 */field++;
++new A()./* @@ label4 */field;


/* @@@ label1 Error Semantic error ESE0209: Cannot assign to this property because it is readonly. */
/* @@@ label2 Error Semantic error ESE0209: Cannot assign to this property because it is readonly. */
/* @@@ label3 Error Semantic error ESE0209: Cannot assign to this property because it is readonly. */
/* @@@ label4 Error Semantic error ESE0209: Cannot assign to this property because it is readonly. */