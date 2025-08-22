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

function isString(test: string): test /* @@ label */is string {
    return typeof test === "string";
}

/* @@@ label Error SyntaxError: 'is' operator is not supported. Use 'instanceof' instead to check whether a variable is instance of a given type and use 'as' operator to cast to the appropriate type! */
