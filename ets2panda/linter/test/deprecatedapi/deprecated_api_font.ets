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
import {font} from './sdk/api/@ohos.font';

@Entry
@Component
struct FontExample {
  fontInfo2: font.FontInfo = font.getFontByName(''); // error
  fontList: Array<string> = new Array<string>();

  aboutToAppear(): void {
    font.registerFont({ // error
      familyName: 'mediumRawFile',
      familySrc: $rawfile('font/medium.ttf')
    })
  }

  build() {
    Column() {
      Button("getFontByName")
        .onClick(() => {
          this.fontInfo2 = font.getFontByName(''); // error
        })
      Button("getSystemFontList")
        .onClick(() => {
          this.fontList = font.getSystemFontList(); // error
        })
      Button("registerFont")
        .onClick(() => {
          font.registerFont({familyName: 'mediumRawFile', familySrc: $rawfile('font/medium.ttf')}); // error
        })
    }.width('100%')
  }
}