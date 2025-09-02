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

@Builder
function MyBuilder(value: string, size: number) {
  Text(value)
    .fontSize(size)
}

@Builder
function YourBuilder(value: string, size: number) {
  Text(value)
    .fontSize(size)
    .fontColor(Color.Pink)
}

const builderArr1: WrappedBuilder<[string, number]>[] = [wrapBuilder(MyBuilder), wrapBuilder(YourBuilder)];
const builderArr2: WrappedBuilder<[string, number]>[] = [wrapBuilder<[string, number]>(MyBuilder), wrapBuilder<[string, number]>(YourBuilder)];
const builderArr3: WrappedBuilder<[string, number]>[] = [new WrappedBuilder(MyBuilder), new WrappedBuilder(YourBuilder)];
const builderArr4: WrappedBuilder<[string, number]>[] = [new WrappedBuilder<[string, number]>(MyBuilder), new WrappedBuilder<[string, number]>(YourBuilder)];

@Entry
@Component
struct Index {
  @Builder
  testBuilder() {
    ForEach(builderArr1, (item: WrappedBuilder<[string, number]>) => {
      item.builder('Hello World', 30)
    })
    ForEach(builderArr2, (item: WrappedBuilder<[string, number]>) => {
      item.builder('Hello World', 30)
    })
    ForEach(builderArr3, (item: WrappedBuilder<[string, number]>) => {
      item.builder('Hello World', 30)
    })
    ForEach(builderArr4, (item: WrappedBuilder<[string, number]>) => {
      item.builder('Hello World', 30)
    })
  }

  build() {
    Row() {
      Column() {
        this.testBuilder()
      }
      .width('100%')
    }
    .height('100%')
  }
}