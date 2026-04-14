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

interface I1 {
    overload foo {f1, f2}
    f1 (a: int): void {}
    f2 (s: string): void {}
}

interface I2 {
    overload foo {f3, f4}
    f3 (a: byte): void {}
    f4 (u: string|int): void {}
}

interface I3 extends I1, I2 {
    overload foo { f1, f2, f3, f4 }  // OK
}

interface I4 extends I1, I2 {
    overload foo { f1, f3 }  // CTE
}

/* @@? 33:5 Error Semantic error ESE0390: Method 'f2' is missed in the overridden explicit overload 'foo', but listed in the parent explicit overload. */
/* @@? 33:5 Error Semantic error ESE0390: Method 'f4' is missed in the overridden explicit overload 'foo', but listed in the parent explicit overload. */
