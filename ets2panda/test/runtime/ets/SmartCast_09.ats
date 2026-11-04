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

class A {
  prev_: A|undefined = undefined;
  next_: A|undefined = undefined;

  m() {
    const prev = this.prev_;
    const next = this.next_;

    if (prev) {
      this.prev_ = undefined;
      prev.next_ = next;
    } 

    if (next) {
      this.next_ = undefined;
      next.prev_ = prev;
    }

  }
}

function main(): void {
}
