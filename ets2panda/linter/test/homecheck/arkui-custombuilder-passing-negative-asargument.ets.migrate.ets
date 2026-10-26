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

@Entry
@Component
struct Index {
  @State show: boolean = false;
  @Builder
  MyBuilder() {
    Text("This is a text");
  }

  build() {
    Button("This is a button")
      .onClick(() => {
        this.show = true;
      })
      .bindSheet($$this.show, this.MyBuilder())
  }
}


@Entry
@Component
struct Index2 {
    @State style: TextContentStyle = TextContentStyle.DEFAULT
    controller: TextInputController = new TextInputController()

    @Builder
    ConstomKeyboardBuilder() {}

    test() {
        TextInput({
            placeholder: '',
            text: '',
            controller: this.controller
        })
            .customKeyboard(this.ConstomKeyboardBuilder(), { supportAvoidance: true })
            .enableKeyboardOnFocus(false)
    }

    build() {}
}
