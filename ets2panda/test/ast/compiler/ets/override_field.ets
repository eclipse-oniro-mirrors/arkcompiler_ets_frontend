/**
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http: //www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

class Base {
    field : number = 0;
    static field1 : number = 0;
}

class A extends Base {
    static override field : number = 1;
    override field1 : number = 1;
    override field2 : number = 1;
    static override field2 : number = 1;
}

/* @@? 22:21 Error TypeError: The 'override' modifier is invalid here because the base class 'Base' does not declare this member. */
/* @@? 23:14 Error TypeError: The 'override' modifier is invalid here because the base class 'Base' does not declare this member. */
/* @@? 24:14 Error TypeError: The 'override' modifier is invalid here because the base class 'Base' does not declare this member. */
/* @@? 25:21 Error TypeError: The 'override' modifier is invalid here because the base class 'Base' does not declare this member. */
