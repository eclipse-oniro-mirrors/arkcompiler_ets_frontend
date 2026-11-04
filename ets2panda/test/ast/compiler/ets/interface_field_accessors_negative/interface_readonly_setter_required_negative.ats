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

interface IWritable {
    value: int
}

class ReadonlyImpl implements IWritable {
    readonly value: int = 1
}

/* @@? 16:1 Error Semantic error ESE0215: Class ReadonlyImpl cannot implement interface IWritable because the declaration type for field value is not compatible with the declaration type required by the interface. */
/* @@? 21:14 Error Semantic error ESE0337: Cannot implement interface IWritable: property 'value' must have a setter, but the implementation is readonly */
