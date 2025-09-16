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
 
import { dragController } from "@kit.ArkUI";
import { image } from '@kit.ImageKit';

@Entry
@Component
struct DragControllerPage {
  @State pixmap: image.PixelMap | null = null

  @Builder
  DraggingBuilder() {
    Column() {
      Text("DraggingBuilder")
        .fontColor(Color.White)
        .fontSize(12)
    }
    .width(100)
    .height(100)
    .backgroundColor(Color.Blue)
  }

  @Builder
  CustomDragView() {
    Column() {
      Text('Drag Me')
        .fontSize(18)
        .fontColor(Color.Black)
    }
    .width(100)
    .height(50)
    .backgroundColor(Color.White)
  }

  private customBuilders: Array<CustomBuilder | DragItemInfo> = [this.CustomDragView];

  build() {
    Column() {
      Button('drag')
        .margin(10)
        .onDragEnter(() => {
          dragController.getDragPreview(); //error
        })
        .onTouch((event?: TouchEvent) => {
          if (event) {
            let dragInfo: dragController.DragInfo = {
              pointerId: 0,
              extraParams: ''
            }
            dragController.createDragAction(this.customBuilders, dragInfo); //error

            dragController.executeDrag(() => { //error
              this.DraggingBuilder()
            }, dragInfo, (err, eve) => {
            });

            dragController.executeDrag(() => { //error
              this.DraggingBuilder()
            }, dragInfo);
          }
        })
    }
  }
}