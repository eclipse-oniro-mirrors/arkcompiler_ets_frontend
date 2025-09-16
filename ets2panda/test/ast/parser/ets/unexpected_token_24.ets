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

function main(): void {
    const switch_arg = 2
    const aa = 1.0;
    with_param() {
        switch /* @@ label1 */switch_arg)
            /* @@ label2 */case aa:
                break
            case 2:
                break
            default: arktest.assertTrue(false, "Not expected execution here");
        }
    }
}

/* @@? 19:5 Error TypeError: Unresolved reference with_param */
/* @@? 20:31 Error SyntaxError: Expected '(', got 'identification literal'. */
/* @@? 21:28 Error SyntaxError: Expected '{', got 'case'. */
