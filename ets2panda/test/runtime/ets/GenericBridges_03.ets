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

interface ZComparable<T> {
  compare(other: T): int;
}

interface XComparable<T, U> extends ZComparable<U> {
  compareTo(other: T): int;
}

abstract class YComparable<T> {
  abstract compareFrom(other: T): int;
}

class V  extends YComparable<V> implements XComparable<V, V> {
  v: int;

  constructor(v: int) { this.v = v; }

  override compareTo(other: V) {
    return this.v - other.v;
  }

  override compareFrom(other: V) {
    return other.v - this.v;
  }

  override compare(other: V) {
    return 2 * other.v - this.v;
  }

}

function main() {
  let v1 = new V(1) as XComparable<V, V>;
  let v2 = new V(1) as YComparable<V>;

  let v3 = new V(2);

  arktest.assertEQ(v1.compareTo(v3), -1);
  arktest.assertEQ(v3.compareTo(v1 as V), 1);

  arktest.assertEQ(v1.compare(v3), 3);
  arktest.assertEQ(v3.compare(v1 as V), 0);

  arktest.assertEQ(v2.compareFrom(v3), 1);
  arktest.assertEQ(v3.compareFrom(v2 as V), -1);
}
