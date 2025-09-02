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

import { Component, State, Row, Slider, $$, Checkbox, Blank, Text } from '@kit.ArkUI';

@Component
struct MyComponent {
  @State value: number = 0

  build() {
    Row() {
      Slider({
        value: $$(this.value)
      })
    }
  }
}

@Component
struct Test {
  @State checked: boolean = false

  build() {
    Row() {
      Checkbox().select($$(this.checked))
      Blank()
      Text(this.checked ? '开启': '关闭')
    }
  }
}