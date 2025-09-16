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

class A { test(): boolean { return true; } }
class B {}

function main(): void {
    let ab: A|B = new A();
    while (ab.test()) {
        ab = new B();
    }
}

/* @@? 21:12 Error TypeError: Member type must be the same for all union objects. */
/* @@? 21:15 Error TypeError: Property 'test' does not exist on type 'B' */
