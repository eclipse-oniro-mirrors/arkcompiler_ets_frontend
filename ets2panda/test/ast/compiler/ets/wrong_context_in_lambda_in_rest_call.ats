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

// A class name used as a value inside a lambda body, where the lambda is nested in a call that is
// itself an argument to the overloaded rest-parameter builtin console.log. Guards two regressions:
// the diagnostic must survive the overload-resolution rollback (the erroneous identifier inside the
// lambda body has to be re-checked), and rest-args lowering must not flatten the reused argument
// source ranges, so the error still points at 'String' instead of at the whole console.log call.

function f(s: Any): void {
}

function main() {
    console.log(
        f(
            () => {
                (typeof 2) === String
            }
        )
    )
}

/* @@? 29:32 Error Syntax error ESY0318: Class cannot be used as object. */
