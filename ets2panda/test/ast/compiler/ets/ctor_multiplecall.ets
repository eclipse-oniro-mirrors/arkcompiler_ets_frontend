/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

class Base {

}

class Derived extends Base {
    constructor() {
        super()
        super()
    }

    constructor(x: number) {
        this()
        this()
    }

    constructor(s: string) {
        super()
        this()
    }
}

/* @@? 23:9 Error Semantic error ESE0611: Duplicate super()/this() call in constructor */
/* @@? 28:9 Error Semantic error ESE0611: Duplicate super()/this() call in constructor */
/* @@? 33:9 Error Semantic error ESE0611: Duplicate super()/this() call in constructor */
