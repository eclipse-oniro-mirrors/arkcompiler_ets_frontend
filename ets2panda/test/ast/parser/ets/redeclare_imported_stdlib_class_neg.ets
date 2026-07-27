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

// Redefining a stdlib-exported name must stay a redeclaration error (not silently shadowed) even though
// stdlib bindings are shared by reference rather than flattened into each program's scope.

class Error /* @@ label */{}

/* @@@ label Error Semantic error ESE0349: Class 'Error' is already defined. */
