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
  protected x: int;
  protected y: float;
  private constructor() {}
  protected constructor(x: int) { this.x = x; }
  public constructor(y: float) {
    this(0);
    this.y = y;
  }
}

final class I {
  protected x: int;
}

class I1 extends /* @@ label */I {
  public constructor() {
    super()
  }
}


/* @@@ label Error Semantic error ESE0178: Cannot inherit with 'final' modifier. */