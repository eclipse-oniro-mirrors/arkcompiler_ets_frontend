/**
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http: //www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

class A<T> {
  fn(t: int): int { return t; }
}

class B extends A<Object> {
  override fn(t: int): void { }
}


/* @@? 21:14 Error Semantic error ESE0141: fn(t: Int): void in B cannot override fn(t: Int): Int in A<T> because overriding return type is not compatible with the other return type. */
/* @@? 21:14 Error Semantic error ESE0136: Method fn(t: Int): void in B not overriding any method */