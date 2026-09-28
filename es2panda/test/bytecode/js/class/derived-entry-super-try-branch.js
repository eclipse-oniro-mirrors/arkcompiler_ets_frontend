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

/* super() inside a try block does not establish the entry-super fact:
 * the this access check and the return check are preserved.
 */
class Base {
    constructor(v) { this.v = v; }
}

class TryBranch extends Base {
    constructor() {
        try { super(); } catch (e) { }
        this.x = 1;
    }
}
