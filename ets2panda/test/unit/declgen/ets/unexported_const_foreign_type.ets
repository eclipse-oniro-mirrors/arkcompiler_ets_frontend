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

/*
 * Regression test: declgen must not crash when it post-dumps an unexported
 * top-level const whose inferred type is declared in a foreign module
 * (std.core). The class property with the same name triggers the post-dump of
 * the const; its checker type has no type annotation, so the declaration scope
 * of the builtin type is walked by compiler::DiffClassScopes. That walk must
 * stop at the foreign module scope instead of dereferencing its null parent.
 */
const FOO = 7;

export class Holder {
    public static FOO: number = 7;
}
