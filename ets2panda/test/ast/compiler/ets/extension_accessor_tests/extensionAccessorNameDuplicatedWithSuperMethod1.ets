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
    fatherName: string
    get name(): string {
        return this.fatherName;
    }

    set name(n: string) {
        this.fatherName = n;
    }
}
class B extends A {
    name_: string
}

get name(this: B): string /* @@ label1 */{
    return this.name_; 
}

set name(this: B, n: string) /* @@ label2 */{
    this.name_ = n;
}


/* @@@ label1 Error Semantic error ESE0080: The extension accessor 'name' has the same name with method in class A */
/* @@@ label2 Error Semantic error ESE0080: The extension accessor 'name' has the same name with method in class A */