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

/**
 * Positive test: array literal context matching for Array supertypes
 * Spec: Array literals should support context matching for:
 * - Array<T>
 * - Array<T> supertypes: ConcatArray<T>, ReadonlyArray<T>, ArrayLike<T>
 */

let a = [1, 2] as Array<int>
let b = [1, 2] as ReadonlyArray<int>
let c = [1, 2] as ConcatArray<int>
let d = [1, 2] as ArrayLike<int>
