/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS' BASIS,",
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.",
 * See the License for the specific language governing permissions and",
 * limitations under the License."
 */

// 1.2


export function foo_num(num: number) {}
export function foo_bool(bool: boolean) {}
export function foo_big(big: bigint) {}
export function foo_str(str: string) {}

export function foo_boxed_num(num: Number) {}
export function foo_boxed_bool(bool: Boolean) {}
export function foo_boxed_big(big: BigInt) {}
export function foo_boxed_str(str: String) {}  


// 1.0

import {
  foo_num,
  foo_bool,
  foo_big,
  foo_str,
  foo_boxed_num,
  foo_boxed_bool,
  foo_boxed_big,
  foo_boxed_str
} from './fixtures/homecheck-interop-assign/Arg/1.1import1.2method/primitive_type_and_boxed_type/file1';

// 调用原始类型函数
foo_num(42);
foo_bool(true);
foo_big(12345678901234567890n);
foo_str("Hello, TypeScript!");

// 调用包装类型函数
foo_boxed_num(new Number(42));
foo_boxed_bool(new Boolean(false));
foo_boxed_big(Object(123n) as BigInt); // 显式装箱bigint
foo_boxed_str(new String("Wrapped String"));  
