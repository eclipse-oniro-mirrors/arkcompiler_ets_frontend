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

import { UIUtils } from '@kit.ArkUI';

interface User {
  name: string;
}

@Entry
@Component
struct Index {
  user: User = UIUtils.makeObserved({ name: 'jack' });
  MyArr: Array<number> = UIUtils.makeObserved(new Array(1, 2));
  MySet: Set<number> = UIUtils.makeObserved(new Set([1, 2, 3]))
  myMap: Map<string, number> = UIUtils.makeObserved(new Map([["jack", 3], ["tom", 2]]));
  myDate: Date = UIUtils.makeObserved(new Date());

  build() {
  }
}