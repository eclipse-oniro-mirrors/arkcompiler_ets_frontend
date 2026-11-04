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

class C {
  x: int;
  y = 1;
  static a: int;
  static b: float;
  readonly d: int = 0;
  //readonly static static readonly readonly static e = 30;
  readonly static e = 30;
  static readonly f = 3.0;
  public static readonly g = 30;
  private static readonly /* @@ label */f = 30;
  protected static readonly h = 30;
}


/* @@@ label Error Semantic error ESE0351: Variable 'f' has already been declared. */