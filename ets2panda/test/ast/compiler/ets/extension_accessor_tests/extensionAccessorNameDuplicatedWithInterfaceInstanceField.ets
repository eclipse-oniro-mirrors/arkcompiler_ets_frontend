/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

interface A {
    name: string
}
class B implements A {
    name = "test"
    name_ : string
}

get name(this: B): string{
    return this.name_; 
}

set name(this: B, n: string) {
    this.name_ = n;
}

/* @@? 24:26 Error Semantic error ESE0080: The extension accessor 'name' has the same name with method in class A */
/* @@? 24:26 Error Semantic error ESE0079: The extension accessor or extension function 'name' has the same name with field of class B */
/* @@? 28:30 Error Semantic error ESE0080: The extension accessor 'name' has the same name with method in class A */
/* @@? 28:30 Error Semantic error ESE0079: The extension accessor or extension function 'name' has the same name with field of class B */