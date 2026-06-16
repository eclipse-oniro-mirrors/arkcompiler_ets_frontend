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

class Example {
    public static foobar(): Object {
        return new Object();
    }

    public static baz(): Object {
        return new Object();
    }
}

function bar(x: int, y: float): Object {
  return new Object();
}

function foo(x: int): Object {
  launch<Object>() {
    return bar(x, 30)
  };
  return new Object();
}

function main(): void {
  let p: Job<Object>;
  p = launch<Object>() {
    return foo(11)
  };
  p = launch(Example.foobar);
  p = launch(Example.baz);
  p = launch(Example.baz);
  let foo_ref: (x: int) => Object = foo;
  p = launch<Object>() {
    return foo_ref(5)
  };
  let lambda: () => void = (): void => { return; }
  launch(lambda);
}
