/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

class C {}

class IR {
    value: C = new C()
}

class CIterator {

  private ind_: int = 0;
  private base_: C[];

  constructor (base: C[]) {
    this.base_ = base;
  }

  next(): IR {
    return {
      value: this.base_[this.ind_ += 1]
    }
  }
}

