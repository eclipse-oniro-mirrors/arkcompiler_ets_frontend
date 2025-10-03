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

// annotations for as expression
let a = 1 as @Anno @Anno() Int

// annotations for instanceof
class C1<@Anno @Anno() T>{}
new C1<Int>() instanceof @Anno @Anno() C1


/* @@? 20:28 Error Syntax error ESY0028: Annotations are not allowed on this type of declaration. */
/* @@? 23:17 Error Semantic error ESE0041: Duplicate annotations are not allowed. The annotation 'Anno' has already been applied to this element. */
/* @@? 24:40 Error Syntax error ESY0028: Annotations are not allowed on this type of declaration. */