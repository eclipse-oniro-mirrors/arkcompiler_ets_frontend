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
 
import { componentSnapshot } from '@kit.ArkUI';

@Entry
@Component
struct SnapshotExample {
  @Builder
  RandomBuilder() {
    Flex() {
      Text('Test menu')
        .fontSize(20)
        .width(100)
        .height(50)
        .textAlign(TextAlign.Center)
    }
    .width(100)
  }

  build() {
    Column() {
      Button("click to generate UI snapshot")
        .onClick(() => {
          componentSnapshot.get("root", () => {}); //error
          componentSnapshot.get("root");  //error
          componentSnapshot.createFromBuilder(() => { this.RandomBuilder()}, () => {}); //error
          componentSnapshot.createFromBuilder(() => { this.RandomBuilder()}); //error
        }).margin(10)
    }
    .width('100%')
    .height('100%')
    .alignItems(HorizontalAlign.Center)
  }
}