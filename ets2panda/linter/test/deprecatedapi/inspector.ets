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
 
import inspector from '@ohos.arkui.inspector'

@Entry
@Component
struct ImageExample {
  build() {
    Column() {
      Flex({ direction: FlexDirection.Column, alignItems: ItemAlign.Start }) {
        Row({ space: 5 }) {
          Image($r('app.media.app_icon'))
            .width(110)
            .height(110)
            .border({ width: 1 })
            .id('IMAGE_ID')
        }
      }
    }.height(320).width(360).padding({ right: 10, top: 10 })
  }

  listener:inspector.ComponentObserver = inspector.createComponentObserver('IMAGE_ID') //error

  aboutToAppear() {
    let onLayoutComplete:()=>void=():void=>{}
    let onDrawComplete:()=>void=():void=>{}
    let FuncLayout = onLayoutComplete // bind current js instance
    let FuncDraw = onDrawComplete // bind current js instance

    this.listener.on('layout', FuncLayout)
    this.listener.on('draw', FuncDraw)
  }
}
