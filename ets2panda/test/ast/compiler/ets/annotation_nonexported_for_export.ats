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

type Version = number[];

@interface my_anno {
    fromVersion: Version;
}

@my_anno([1, 1])
export function bar() {}


/* @@? 22:1 Error Semantic error ESE71336: Exported function 'bar' requires type annotation. */
/* @@? 22:2 Error Semantic error ESE1236581: Type 'my_anno' used in export declaration(s) is not exported. */