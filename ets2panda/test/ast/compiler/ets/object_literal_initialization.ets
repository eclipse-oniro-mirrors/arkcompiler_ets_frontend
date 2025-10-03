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

// arkts-no-untyped-obj-literals
let o: Object = /* @@ label1 */{/* @@ label2 */n: 42, s: "foo"}


/* @@@ label1 Error Semantic error ESE115343: 'Object', 'object', 'any' types cannot be initialized with object literals. */
/* @@@ label2 Error Semantic error ESE0065: type Object has no property named n */