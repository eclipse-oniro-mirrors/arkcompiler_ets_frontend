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
    testProperty2: string
}

@MyAnno({testProperty1:"", testProperty2: "", testProperty3: "", testProperty4: ""})
interface itf {
    @MyAnno({testProperty1:"", testProperty2: "", testProperty3: "", testProperty4: ""})
    foo(): void
    @MyAnno({testProperty1:"", testProperty2: "", testProperty3: "", testProperty4: ""})
    x : int
}


/* @@? 21:2 Error Semantic error ESE0068: The number of arguments provided for the annotation exceeds the number of fields defined. */
/* @@? 23:6 Error Semantic error ESE0068: The number of arguments provided for the annotation exceeds the number of fields defined. */
/* @@? 25:6 Error Semantic error ESE0068: The number of arguments provided for the annotation exceeds the number of fields defined. */