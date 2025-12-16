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

import { Text } from './sdk/api/@internal/component/ets/common'
import { Circle } from './sdk/api/@internal/component/ets/circle'

@Entry
@Component
struct MyComponent {
  build() {
    Column() {
      Text('Hello world')
        .clip(new Circle({ width: 10, height: 10 }))

      Text('Hello world')
        .clip(new Circle({ width: 10, height: 10 }))

      Text('Hello world')
        .width(50)
        .clip(new Circle({ width: 10, height: 10 }))

      Text('Hello world')
        .clip(new Circle({ width: 10, height: 10 }))
        .width(50)

      Text('Hello world')
        .height(50)
        .clip(new Circle({ width: 10, height: 10 }))
        .width(50)
    }
  }
}