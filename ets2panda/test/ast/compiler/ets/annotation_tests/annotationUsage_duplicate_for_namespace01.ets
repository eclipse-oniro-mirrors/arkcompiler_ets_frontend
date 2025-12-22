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

@interface Anno1{}
@interface Anno2{}
@interface Anno3{}

@Anno1
@Anno1
namespace A.B{}

@Anno2
@Anno2
namespace A{
    @Anno3
    @Anno3
    namespace C{}
}

/* @@? 21:2 Error Semantic error ESE0041: Duplicate annotations are not allowed. The annotation 'Anno1' has already been applied to this element. */
/* @@? 25:2 Error Semantic error ESE0041: Duplicate annotations are not allowed. The annotation 'Anno2' has already been applied to this element. */
/* @@? 28:6 Error Semantic error ESE0041: Duplicate annotations are not allowed. The annotation 'Anno3' has already been applied to this element. */