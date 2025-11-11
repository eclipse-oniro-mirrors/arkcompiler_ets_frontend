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

function id<T>(v: T): T { return v; }

function returnBad() { return id; }

function main() {
  let g = id;
}

/* @@? 18:31 Error Semantic error ESE655063: Generic function id requires explicit type arguments when used as a value. */
/* @@? 21:11 Error Semantic error ESE655063: Generic function id requires explicit type arguments when used as a value. */