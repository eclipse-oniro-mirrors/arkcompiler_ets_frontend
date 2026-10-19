/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

class B {
  v = 2
}

const array: B[] = [ {v: 2}, {v: 3}, {v: 4} ];
function arr() {
  let _ = array[0] instanceof B;
}

class B2 {
  v = 2
}

function arr2() {
  const array: B[] = [ {v: 2}, {v: 3}, {v: 4} ];
  let _ = array[0] instanceof B;
}
