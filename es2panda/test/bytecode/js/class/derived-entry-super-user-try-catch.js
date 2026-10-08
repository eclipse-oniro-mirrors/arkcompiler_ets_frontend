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

/* The user try/catch handler survives; this accesses inside the try and
 * catch bodies skip the redundant 0x0 checks.
 */
class Base {
    constructor(v) { this.v = v; }
}

class UserTryCatch extends Base {
    constructor(fail) {
        super();
        try {
            if (fail) { throw new Error("body-failed"); }
            this.x = 1;
        } catch (e) { this.x = e.message; }
    }
}
