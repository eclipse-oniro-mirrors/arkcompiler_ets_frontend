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

class B {
    name_: string
    constructor(n: string) {
        this.name_ = n;
    }
}

get name(this: B): string {
    return this.name_; 
}

set name(this: B, n: string) {
    this.name_ = n;
}

function main() {
    let b = new B("test");
    /* @@ label */b.name();
}


/* @@@ label Error Semantic error ESE0059: Extension accessor can't be used as a method call or function call. */