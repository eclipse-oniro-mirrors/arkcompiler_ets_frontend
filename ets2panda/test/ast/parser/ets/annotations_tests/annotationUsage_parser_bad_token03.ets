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
@interface MyAnno {
    testProperty1: string
    testProperty2: number
}

@MyAnno({testProperty1: "123", testProperty2: 123/* @@ label */;/* @@ label1 */})
class A{}


/* @@? 20:2 Error Semantic error ESE0068: The number of arguments provided for the annotation exceeds the number of fields defined. */
/* @@@ label Error Syntax error ESY0230: Expected ',', got ';'. */
/* @@@ label Error Syntax error ESY0224: Identifier expected, got ';'. */
/* @@@ label1 Error Syntax error ESY0224: Identifier expected, got '}'. */