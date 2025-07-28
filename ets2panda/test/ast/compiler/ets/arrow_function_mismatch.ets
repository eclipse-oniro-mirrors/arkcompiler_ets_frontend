/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

let foo: (P: string, Q: string) => void

foo((value: Object): void => { }, (err: Error): void => { })

/* @@? 18:1 Error TypeError: No matching call signature for ((p1: Object) => void, (p1: Error) => void) */
/* @@? 18:5 Error TypeError: Type '(p1: Object) => void' is not compatible with type 'String' at index 1 */
