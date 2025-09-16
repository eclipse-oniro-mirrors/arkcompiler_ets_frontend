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
import { matrix4 } from './sdk/api/@ohos.matrix4';

@Entry
@Component
struct Test {
  private matrix1 = matrix4.rotate({   // error
      x: 1,
      y: 1,
      z: 2,
      angle: 30
    });
  private matrix1 = matrix4.copy();    // error

  private matrix1 = matrix4.translate({ x: 100 });    // error

  build() {
    Column() {
    }.width("100%").margin({ top: 50 })
  }
}