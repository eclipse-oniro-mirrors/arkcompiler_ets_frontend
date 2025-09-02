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

let a: number[] = [1.0, 2.0, 3.0]
for (let i /* @@ label */in a) {
}

/* @@@ label Error SyntaxError: Variable must be initialized or it's type must be declared. */
/* @@@ label Error SyntaxError: 'for ... in' loop is not supported, please use regular 'for' or 'for ... of ...' loop to iterate through arrays
    and iterable objects. */
