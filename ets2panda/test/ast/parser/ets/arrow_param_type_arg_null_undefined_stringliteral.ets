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

class A<T, V> {}

class B<T, V> {}

class C<T> {}

/* Regression: comma in generic args must not break arrow-function paren probe (null is LITERAL_NULL). */
let check_null = (v: A<int, null> | B<number, string> | C<string> | bigint, id: int) => {}
let check_undefined = (v: A<int, undefined> | B<number, string> | C<string> | bigint, id: int) => {}
let check_string_literal = (v: A<int, 'test_literal'> | B<number, string> | C<string> | bigint, id: int) => {}
