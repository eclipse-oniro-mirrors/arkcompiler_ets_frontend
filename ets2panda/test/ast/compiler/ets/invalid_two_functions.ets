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

(function () {})();

const a = [1];
const b = a.map(function (e) {
    return e * 2;
});


/* @@? 16:2 Error Syntax error ESY0320: Function expressions are not supported, use arrow functions instead */
/* @@? 19:17 Error Syntax error ESY0320: Function expressions are not supported, use arrow functions instead */
/* @@? 19:28 Error Syntax error ESY0194: Parameter declaration should have an explicit type annotation. */