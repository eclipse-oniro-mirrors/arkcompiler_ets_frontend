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

let tupleArray: any = [1, null, "hello", true, null]
let arr: any[] = [1, null, "hello", true, null]
let arr2: any[] = arr

/* @@? 16:17 Error Semantic error ESE153545: 'any' type annotation is forbidden */
/* @@? 17:10 Error Semantic error ESE153545: 'any' type annotation is forbidden */
/* @@? 18:11 Error Semantic error ESE153545: 'any' type annotation is forbidden */
