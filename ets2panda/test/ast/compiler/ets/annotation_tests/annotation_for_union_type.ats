/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

@Retention("SOURCE")
@interface Anno{}

//  // @Message for 'Int' and @Anno2 for 'String'
let unionType1: @Anno() @Anno() Int | @Anno() @Anno 'String'

// @Message for 'Int | String' and @Anno2 for 'String'
let unionType2: @Anno() @Anno() (Int | @Anno @Anno String)

// @Message for 'Int | String' and @Anno2 for 'Int'
let unionType3: @Anno() @Anno() (@Anno @Anno Int | String)

// @Message for 'Int | String', @Anno1 for 'Int' and @Anno2 for 'String'
let unionType4: @Anno() @Anno() (@Anno @Anno Int | @Anno @Anno String)


/* @@? 20:26 Error Semantic error ESE0041: Duplicate annotations are not allowed. The annotation 'Anno' has already been applied to this element. */
/* @@? 20:48 Error Semantic error ESE0041: Duplicate annotations are not allowed. The annotation 'Anno' has already been applied to this element. */
/* @@? 23:26 Error Semantic error ESE0041: Duplicate annotations are not allowed. The annotation 'Anno' has already been applied to this element. */
/* @@? 23:47 Error Semantic error ESE0041: Duplicate annotations are not allowed. The annotation 'Anno' has already been applied to this element. */
/* @@? 26:26 Error Semantic error ESE0041: Duplicate annotations are not allowed. The annotation 'Anno' has already been applied to this element. */
/* @@? 26:41 Error Semantic error ESE0041: Duplicate annotations are not allowed. The annotation 'Anno' has already been applied to this element. */
/* @@? 29:26 Error Semantic error ESE0041: Duplicate annotations are not allowed. The annotation 'Anno' has already been applied to this element. */
/* @@? 29:41 Error Semantic error ESE0041: Duplicate annotations are not allowed. The annotation 'Anno' has already been applied to this element. */
/* @@? 29:59 Error Semantic error ESE0041: Duplicate annotations are not allowed. The annotation 'Anno' has already been applied to this element. */