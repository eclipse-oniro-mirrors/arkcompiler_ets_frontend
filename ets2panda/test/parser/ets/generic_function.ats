/*
 * Copyright (c) 2022 - 2024 Huawei Device Co., Ltd.
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

interface Comparable2 {
  localeCompare(other: Object|null): int;
}

function compare<T extends Comparable2>(x: T|null, y: T|null): int {
  if (x == null) return y == null ? 0 : -1;
  if (y == null) return 1;
  return x.localeCompare(y);
}
