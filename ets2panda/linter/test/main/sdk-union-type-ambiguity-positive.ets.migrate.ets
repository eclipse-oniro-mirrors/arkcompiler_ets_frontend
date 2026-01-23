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

import {
  Entry,
  Component,
  RelativeContainer,
  Text,
  $r,
  FontWeight,
  VerticalAlign,
  HorizontalAlign,
  ColumnOptions,
  RowOptions,
} from '@kit.ArkUI';

import { Column, Row } from './sdk/column_row';

// Test case 1: Column with only common field (space) - triggers sdk-union-type-ambiguity error in ArkTS-2 mode
Column({ space: 20 } as ColumnOptions)


// Test case 2: Row with only common field (space) - triggers sdk-union-type-ambiguity error in ArkTS-2 mode
Row({ space: 20 } as RowOptions)


// Test case 3: Column with space as string - triggers sdk-union-type-ambiguity error in ArkTS-2 mode
Column({ space: '10' } as ColumnOptions)


// Test case 4: Row with space as number - triggers sdk-union-type-ambiguity error in ArkTS-2 mode
Row({ space: 10 } as RowOptions)


@Entry
@Component
struct Index {
  message: string = 'Hello World';

  build() {
    RelativeContainer() {
      Column({space: 5} as ColumnOptions)  // Test case 5: Column in component build() - triggers sdk-union-type-ambiguity error in ArkTS-2 mode
      Text(this.message)
        .id('HelloWorld')
        .fontSize($r('app.float.page_text_font_size'))
        .fontWeight(FontWeight.Bold)
        .alignRules({
          center: { anchor: '__container__', align: VerticalAlign.Center },
          middle: { anchor: '__container__', align: HorizontalAlign.Center }
        })
        .onClick(() => {
          this.message = 'Welcome';
        })
    }
    .height('100%')
    .width('100%')
  }

}