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

@Entry
@Component
struct GridContainerExample {
  @State sizeType: SizeType = SizeType.MD // error
  private options: GridContainerOptions = {
    columns: 12, // error
    sizeType: this.sizeType, // error
    gutter: 10, // error
    margin: 20  // error
  }

  build() {
    Column({ space: 5 }) {
      GridContainer({
        columns: 12,
        sizeType: this.sizeType,
        gutter: 10,
        margin: 20
      }) {
        Row() {
          Text('1')
            .height(50).backgroundColor(0x4682B4).textAlign(TextAlign.Center)
        }
      }.width('90%')

      GridContainer(this.options) {
        Row() {
          Text('1')
            .useSizeType({
              xs: { span: 6, offset: 0 },
              sm: { span: 2, offset: 0 },
              md: { span: 2, offset: 0 },
              lg: { span: 2, offset: 0 }
            })
            .height(50).backgroundColor(0x4682B4).textAlign(TextAlign.Center)
        }
      }.width('90%')

      Text('Click Simulate to change the device width').fontSize(9).width('90%').fontColor(0xCCCCCC)
      Row() {
        Button('LG')
          .onClick(() => {
            this.sizeType = SizeType.LG // error
          }).backgroundColor(0x317aff)
        Button('Auto')
          .onClick(() => {
            this.sizeType = SizeType.Auto // error
          }).backgroundColor(0x317aff)
      }
    }.width('100%').margin({ top: 5 })
  }
}