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

class CLterator implements Iterator <string> /* @@ label */{
    next(): Iterator<string> {}
}



/* @@@ label Error Semantic error ESE0190: CLterator is not abstract and does not override abstract method next(): IteratorResult<String> in Iterator */
/* @@? 17:9 Error Semantic error ESE0141: next(): Iterator<String> in CLterator cannot override next(): IteratorResult<String> in Iterator<T> because overriding return type is not compatible with the other return type. */
/* @@? 17:9 Error Semantic error ESE0136: Method next(): Iterator<String> in CLterator not overriding any method */