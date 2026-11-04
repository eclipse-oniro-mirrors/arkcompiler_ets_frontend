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

function choose(useString: boolean): string | [string, string] {
    let tuple: [string, string] = ["x", "y"]
    return useString ? "xy" : tuple
}

function main(): void {
    let source: string | [string, string] = choose(false)
    let copied: string[] = [...source]
}

/* @@? 23:29 Error Semantic error ESE0049: The spread expression of type 'String|[String, String]' must be of iterable or tuple type. */
