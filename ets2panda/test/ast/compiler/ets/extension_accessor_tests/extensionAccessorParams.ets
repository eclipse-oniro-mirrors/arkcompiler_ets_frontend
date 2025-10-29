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

/* @@ label1 */get field(this: A, a: int): int {
    return this.field_
}

/* @@ label2 */set field(this: A, a: int, b: int) {

}


/* @@@ label1 Error Syntax error ESY0022: Extension Getter can only have 1 parameter. */
/* @@@ label2 Error Syntax error ESY0023: Extension Setter can only have 2 parameters. */