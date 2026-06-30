/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

let valueBinding: int = 1

function functionBinding(): int {
    return 1
}

@Retention({policy: "SOURCE"})
@interface AnnotationBinding {}

export type { valueBinding }
export type { functionBinding }
export type { AnnotationBinding }

/* @@? 25:15 Error Semantic error ESE0371: Cannot find type 'valueBinding'. */
/* @@? 26:15 Error Semantic error ESE0371: Cannot find type 'functionBinding'. */
/* @@? 27:15 Error Semantic error ESE0159: Annotations cannot be used as a type. */
