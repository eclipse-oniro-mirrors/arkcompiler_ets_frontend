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

class B<T = Object> {}

function gen<T = Object>(p: T): T { return p }

// Empty type argument list "<>" is not a valid type reference.
type A = B<>;
let b = new B<>();
let c: B<> = new B();

// Empty type argument list "<>" is not a valid generic function reference.
let r = gen<>;

/* @@? 21:11 Error Syntax error ESY31671: Type argument list cannot be empty. */
/* @@? 22:14 Error Syntax error ESY31671: Type argument list cannot be empty. */
/* @@? 23:9 Error Syntax error ESY31671: Type argument list cannot be empty. */
/* @@? 26:12 Error Syntax error ESY31671: Type argument list cannot be empty. */
