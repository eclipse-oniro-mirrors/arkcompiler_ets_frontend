/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

import { Text, Column, Component, Entry, Button, ClickEvent } from "@ohos.arkui.component"
import { State, Link, Prop } from "@ohos.arkui.stateManagement"
import hilog from '@ohos.hilog'

@Entry
@Component
struct MyStateSample {
  @State stateVar: string = "state var";
  message: string = `click to change state variable, add **`;
  changeValue() {
    this.stateVar+="**"
  }
  build() {
    Column() {
      Button("clean variable").onClick((e: ClickEvent) => { this.stateVar = "state var" })
      Text("Hello World").fontSize(20)
      Button(this.message).backgroundColor("#FFFF00FF")
        .onClick((e: ClickEvent) => {
          hilog.info(0x0000, 'testTag', 'On Click');
          this.changeValue()
        })
      Text(this.stateVar).fontSize(20)
      Child({linkVar: this.stateVar, propVar: this.stateVar})
    }.margin(10)
  }
}

@Component
struct Child {
  @Link linkVar: string = ""; // TODO: remove this
  @Prop propVar: string = "Prop";

  changeValue1() {
    this.linkVar+="!!"
  }

  changeValue2() {
    this.propVar+="~~"
  }

  build() {
    Column() {
      Button(`click to change Link variable, add symbol !!`)
        .backgroundColor("#4169E1")
        .onClick((e: ClickEvent) => {
          hilog.info(0x0000, 'testTag', 'On Click');
          this.changeValue1()
        })
      Button(`click to change Prop variable, add symbol ~~`)
        .backgroundColor("#3CB371")
        .onClick((e: ClickEvent) => {
          hilog.info(0x0000, 'testTag', 'On Click');
          this.changeValue2()
        })
      Text(`Link variable in child: ${this.linkVar}`).fontSize(30)
      Text(`Prop variable in child: ${this.propVar}`).fontSize(30)
    }
  }
}