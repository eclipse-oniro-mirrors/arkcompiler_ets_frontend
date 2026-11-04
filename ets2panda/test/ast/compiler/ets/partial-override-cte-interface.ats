/**
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

interface Base {
    foo(v: string | null): void
}

interface Derived extends Base {
    foo(v: Object): void
}

/* @@? 21:5 Error Semantic error ESE1029298: Member foo in Derived cannot override foo in Base because a parameter type of the overridden method contains a nullish value that is not a subtype of the corresponding parameter type of the overriding method, while the non-nullish part of that parameter type is non-empty and is a subtype of it. */
