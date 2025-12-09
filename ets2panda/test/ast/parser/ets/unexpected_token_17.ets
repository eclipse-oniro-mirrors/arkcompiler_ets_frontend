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
    with_param() {
        switch (switch_arg) 
            /* @@ label */case aa:
                break
            case 2:
                break
            default: arktest.assertTrue(false, "Not expected execution here");
        }
    }
}


/* @@? 23:18 Error Semantic error ESE0091: Type 'void' is not compatible with the enclosing method's return type 'Int' */
/* @@@ label Error Syntax error ESY0230: Expected '{', got 'case'. */
/* @@? 25:32 Error Semantic error ESE0262: Switch case type 'double' is not comparable to discriminant type 'int' */