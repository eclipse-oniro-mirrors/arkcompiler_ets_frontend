/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

@Component
struct MyCard {
  build() {
    Column() {
      Text('Card')
    }
    .cardStyle()
  }
}

const mycolor: string = "#ffffff"

@Extend(Column)
function cardStyle()
{
  .backgroundColor("#ffff00")
  .backgroundColor("#00ffff")
  .backgroundColor("#ff00ff")
  .backgroundColor(mycolor)
  .backgroundColor(Color.Red)
  .borderRadius(8)
  .padding(8)
  .backgroundImagePosition({
    x: 0,
    y: 0
  });
}

@Extend(Column)
function superCard(padding: number)
{
  .cardStyle()
  .padding(10)
}