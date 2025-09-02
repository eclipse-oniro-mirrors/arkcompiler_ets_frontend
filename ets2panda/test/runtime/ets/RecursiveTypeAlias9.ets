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

type A<T> = T | Array<T> | Array<A<T>>;

function main(): int {
  let a: A<string> = "test";

  a = new Array<A<string>>(a, new Array<string>("test1", "test2"), new Array<A<string>>(a, new Array<A<string>>(a))) as A<string>;

  arktest.assertEQ((a as Array<A<string>>)[0], "test")
  arktest.assertEQ(((a as Array<A<string>>)[1] as Array<A<string>>)[0], "test1")
  arktest.assertEQ(((a as Array<A<string>>)[1] as Array<A<string>>)[1], "test2")
  arktest.assertEQ(((a as Array<A<string>>)[2] as Array<A<string>>)[0], "test")
  arktest.assertEQ((((a as Array<A<string>>)[2] as Array<A<string>>)[1] as Array<A<string>>)[0], "test")

  return 0;
}