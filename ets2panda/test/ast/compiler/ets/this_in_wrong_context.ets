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

export declare @interface ClassAuthor {
    color: Color[] = [this.listener !== undefined]
}

/* @@? 1:3 Error Semantic error ESE0371: Cannot find type 'Color'. */
/* @@? 17:5 Error Semantic error ESE0042: Invalid annotation field type. Only numeric, boolean, string, enum, or arrays of these types are permitted for annotation fields. */
/* @@? 17:22 Error Semantic error ESE0012: Invalid value for annotation field, expected a constant literal. */
/* @@? 17:23 Error Semantic error ESE0203: Cannot reference 'this' in this context. */
/* @@? 17:23 Warning Warning W0010: The instance field initializer expression cannot use the this. */
