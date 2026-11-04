/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

export function foo1() { }
function foo2() { }

overload bar1 {foo1}
overload bar2 {foo1, foo2}
export overload bar3 { /* @@ label1 */foo2 }
export overload bar4 { foo1, /* @@ label2 */foo2 }

/* @@? 16:1 Error Semantic error ESE71336: Exported function 'foo1' requires type annotation. */
/* @@@ label1 Error Semantic error ESE0456: The overload function must be exported along because the overload alias is exported. */
/* @@@ label2 Error Semantic error ESE0456: The overload function must be exported along because the overload alias is exported. */
/* @@? 20:10 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '() => undefined' will never be selected for a call, as the preceding entity with the signature '() => undefined' prevails. */
/* @@? 22:17 Warning Warning W2323: The order of entities in an overload set implies that the overloaded entity with the signature '() => undefined' will never be selected for a call, as the preceding entity with the signature '() => undefined' prevails. */
