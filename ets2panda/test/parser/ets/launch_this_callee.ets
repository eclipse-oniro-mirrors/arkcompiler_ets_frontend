/*
 * Copyright (c) 2024-2026 Huawei Device Co., Ltd.
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

import {launch} from "std/concurrency"
import {Job} from "std/core"

class Foo {
  public  bar(): Job<Foo> {
    return launch<Foo>() {
      return this.baz(this)
    };
  }

  public  baz(x: Foo): Foo {
    return x;
  }

  public  bar2(): Job<Foo> {
    return launch<Foo>() {
      return this.baz2(this, 1)
    };
  }

  public  baz2(x: Foo, y: int): Foo {
    return x;
  }
}
