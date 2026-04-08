/*
 * Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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

interface Inter {
    a ?: number
}

class Klass implements Inter {
    a ?: string
}

/* @@? 21:5 Error Semantic error ESE985118: Class member a with type undefined|String in Klass cannot implement a with type Double|undefined property from interface Inter because the implementing return type is not compatible with the interface return type. */
/* @@? 21:5 Error Semantic error ESE0136: Method a(): undefined|String in Klass not overriding any method */
