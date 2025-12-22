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

class A{
    field_: int;
}

/* @@ label1 */get field(a: A): int {
    return a.field_;
}

/* @@ label2 */set field() {}


/* @@@ label1 Error Syntax error ESY0058: Getter must not have formal parameters. */
/* @@@ label2 Error Syntax error ESY0063: Setter must have exactly one formal parameter. */
