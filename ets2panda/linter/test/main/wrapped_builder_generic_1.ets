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

const wrappedBuilder1: WrappedBuilder<[string, number]> = wrapBuilder(MyBuilder);
const wrappedBuilder2: WrappedBuilder<[string, number]> = wrapBuilder<[string, number]>(MyBuilder);
const wrappedBuilder3: WrappedBuilder<[string, number]> = new WrappedBuilder(MyBuilder);
const wrappedBuilder4: WrappedBuilder<[string, number]> = new WrappedBuilder<[string, number]>(MyBuilder);

@Entry
@Component
struct TestWrappedBuilder1 {
  @State message: string = 'Hello World';

  build() {
    Row() {
      Column() {
        wrappedBuilder1.builder(this.message, 50)
        wrappedBuilder2.builder(this.message, 50)
        wrappedBuilder3.builder(this.message, 50)
        wrappedBuilder4.builder(this.message, 50)
      }
    }
    .height('100%')
    .width('100%')
  }
}