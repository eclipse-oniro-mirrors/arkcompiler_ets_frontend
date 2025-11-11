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

function id<T>(x: T): T { return x; }
function use<T>(f: (x: T) => T, v: T): T { return f(v); }

function good<T>(v: T): T {
  return use<T>(id<T>, v);
}

function bad<T>(v: T): T {
  return use<T>(id, v);
}

function main() {
  arktest.assertEQ(good<Int>(5), 5);
}

/* @@? 24:17 Error Semantic error ESE655063: Generic function id requires explicit type arguments when used as a value. */