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

class C {
  data: int[] = [1,2,3]
  static $_iterator() {
      return new CIterator(new C())
  }
}
class CIterator implements Iterator<int> {
  index = 0
  base: C
  constructor (base: C) {
      this.base = base
  }
  next(): IteratorResult<int> {
      return {
          done: this.index >= this.base.data.length,
          value: this.index >= this.base.data.length ? undefined : this.base.data[this.index++]
      }
  }
}

function main(): int {
  let c = new C()
  let res = ''
  for (let x of c) {
      res += x
  }
  if (res != '6') return 1
  return 0
}


/* @@? 39:17 Error Semantic error ESE0207: '$_iterator' is a static property of 'C' */
/* @@? 39:17 Error Semantic error ESE0076: 'For-of' statement source expression is not of iterable type. */