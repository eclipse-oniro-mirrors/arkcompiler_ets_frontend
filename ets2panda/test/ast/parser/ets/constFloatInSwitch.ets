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

function with_param(fn: () => int): void {
    fn()
}

function main(): void {
    const switch_arg = 2
    const aa = 1.0;
    with_param() /* @@ label1 */{
        switch (switch_arg) {
            /* @@ label2 */case aa:
                break
            case 2:
                break
            default: arktest.assertTrue(false, "Not expected execution here");
        }
    }
}

/* @@@ label1 Error TypeError: Type 'void' is not compatible with the enclosing method's return type 'Int' */
/* @@? 25:33 Error TypeError: Switch case type 'double' is not comparable to discriminant type 'int' */
