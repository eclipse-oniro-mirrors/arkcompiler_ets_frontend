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

import { Component, Column, Text, ColumnAttribute, Color } from '@kit.ArkUI';

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

function cardStyle(this: ColumnAttribute): this {
  this.backgroundColor("#ffff00");
  this.backgroundColor("#00ffff");
  this.backgroundColor("#ff00ff");
  this.backgroundColor(mycolor);
  this.backgroundColor(Color.Red);
  this.borderRadius(8);
  this.padding(8);
  this.backgroundImagePosition({
    x: 0,
    y: 0
  });
  return this;
}

function superCard(this: ColumnAttribute, padding: number): this {
  .cardStyle()
  .padding(10)
}