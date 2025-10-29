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

interface I {
  f(x: int): int;
}

let f = (this: I, x: int): int => x;

function main() {
  let obj: I = {
    /* @@ label */f: f
  }

  let r: int = obj.f(5);
  arktest.assertEQ(r, 5);
}


/* @@@ label Error Semantic error ESE0074: Class or interface methods cannot be initialized within an object literal. */