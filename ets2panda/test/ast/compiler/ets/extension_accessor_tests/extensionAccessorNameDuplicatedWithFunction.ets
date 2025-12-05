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

class B {
    name_: string
}

function name(b: B) {}
function name(b: B, n: string) {}

get name(this: B): string {
    return this.name_;
}

/* @@ label */set name(this: B, n: string) {
    this.name_ = n;
}

/* @@? 20:10 Error Semantic error ESE0218: Method cannot use the same name as name accessor property */
/* @@@ label Error Semantic error ESE0131: Function name with this assembly signature already declared. */