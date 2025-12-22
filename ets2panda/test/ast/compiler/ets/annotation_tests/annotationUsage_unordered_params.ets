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

@MyAnno({testProperty2: /* @@ label */"Bob", testProperty1: /* @@ label1 */1})
class A{}


/* @@@ label Error Semantic error ESE0318: Type '"Bob"' cannot be assigned to type 'Double' */
/* @@@ label1 Error Semantic error ESE0318: Type 'Int' cannot be assigned to type 'String' */