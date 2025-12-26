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

@interface MyAnno{} 

@MyAnno()
@interface Log{}

@Retention("CLASS")
class A{}

@Retention("ADADAFAF")
function foo(){}

@Retention("ADADAFAF")
@interface Message{}

@Retention("RUNTIME")
@interface ClassAuthor{} // Ok

/* @@? 18:2 Error Semantic error ESE0006: Only standard annotations are allowed to be applied on annotations. */
/* @@? 21:2 Error Semantic error ESE1122645: Annotation '@Retention' can only be applied to annotation declarations. */
/* @@? 24:2 Error Semantic error ESE1122645: Annotation '@Retention' can only be applied to annotation declarations. */
/* @@? 27:12 Error Semantic error ESE0004: Invalid value for 'policy' field. The policy must be one of the following:'SOURCE', 'CLASS', or 'RUNTIME'. */