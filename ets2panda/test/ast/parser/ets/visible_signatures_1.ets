/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
    private foo(a: Double | undefined) : int {
        return 0;
    }
    private foo(a: double) : int {
        return 1;
    }
}

function main(): void {
    new A().foo(2.0)
}


/* @@? 20:13 Error TypeError: Function foo with this assembly signature already declared. */
/* @@? 26:5 Error TypeError: Signature foo(a: Double|undefined): Int is not visible here. */
/* @@? 26:5 Error TypeError: No matching call signature for foo(Double) */
