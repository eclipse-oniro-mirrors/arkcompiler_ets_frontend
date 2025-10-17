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

import {
  Entry,
  Component,
  CommonMethod,
  Column,
  Text,
  Color,
  CustomStyles,
  applyStyles,
} from '@kit.ArkUI';

@Entry
@Component
struct MyCard {
  private cardStyle1: CustomStyles = (instance: CommonMethod): void => {
    instance.backgroundColor('#ffffff');
    instance.borderRadius(8);
  };

  public cardStyle2: CustomStyles = (instance: CommonMethod): void => {
    instance.backgroundColor('#ffffff');
    instance.borderRadius(8);
  };

  build() {
    Column() {
      Text('Card').applyStyles(this.cardStyle1)
    }
    .applyStyles(this.cardStyle2)
    .backgroundColor(Color.Red)
  }
}