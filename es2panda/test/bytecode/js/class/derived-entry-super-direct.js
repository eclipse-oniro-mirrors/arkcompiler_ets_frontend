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

/* The entry super() dominates later this/super property accesses and
 * the normal return, so the constant-passing throw.ifsupernotcorrectcall 0x0
 * checks and the exclusive return-check try/catch scaffolding are not
 * emitted; the repeated-call 0x1 check stays.
 */
class Base {
    constructor(v) { this.v = v; }
}

class Direct extends Base {
    constructor(v) {
        super(v);
        this.x = v;
        this.y = v;
        this.z = super.n;
    }
}
