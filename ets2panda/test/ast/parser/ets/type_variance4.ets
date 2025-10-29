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

// TODO: Should be a "SyntaxError: Variance modifier is not allowed here [type_variance4.ets:18:13]",
// after generic global function parsing is implemented.
function fn</* @@ label */in T>(t: T): void {}


/* @@@ label Error Syntax error ESY0098: Variance modifier is not allowed here. */