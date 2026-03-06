/**
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

import { annotation, /* @@ label1 */stub } from 'arkruntime'

@/* @@ label2 */annotation.Async
class X {
    p: number = 0
}

/* @@ label3 */stub;

/* @@@ label1 Error Semantic error ESE154780: 'stub' cannot be used here as it is an arkruntime internal API */
/* @@@ label2 Error Semantic error ESE154780: 'Async' cannot be used here as it is an arkruntime internal API */
/* @@@ label3 Error Syntax error ESY0318: Namespace cannot be used as object. */
/* @@@ label3 Error Semantic error ESE154780: 'stub' cannot be used here as it is an arkruntime internal API */
