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

// Missing a class name: not valid ArkTS syntax. gluegen must still exit 0 and write a
// {"status": "syntax-error"} --output -- see native/src/gluegen.cpp's Gluegen::Run(): a source
// syntax error is a normal, terminal outcome, not a tool failure (compare negative-cases/, whose
// cases are all about gluegen's own usage/environment, never about the input source's validity).
export class {}
