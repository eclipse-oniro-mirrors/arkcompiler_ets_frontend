/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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
namespace NS {
    function bar(): void {}
    export { baz }
}

NS.baz();

/* @@? 17:14 Error Syntax error ESY0279: Cannot find name 'baz' to export. */
/* @@? 20:4 Error Semantic error ESE0087: Property 'baz' does not exist on type 'NS' */
