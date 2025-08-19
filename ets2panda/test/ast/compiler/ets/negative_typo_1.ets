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

@interface Anno {
  e: @ring[][] = [["a", (c +ue)]]
}

/* @@? 17:3 Error TypeError: Invalid annotation field type. Only numeric, boolean, string, enum, or arrays of these types are permitted for annotation fields. */
/* @@? 17:7 Error TypeError: Cannot find type 'ring'. */
/* @@? 17:18 Error TypeError: Invalid value for annotation field, expected a constant literal. */
/* @@? 17:19 Error TypeError: Initializer has 2 elements, but tuple requires 0 */
