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

import { matrix4 } from '@kit.ArkUI';

@Entry
@Component
struct Test {
  private matrix1 = matrix4
    .scale({
      // error
      x: 2,
      y: 3,
      z: 4,
      centerX: 50,
      centerY: 50
    });
  private originPoint: number[] = [50, 50];
  private transformPoint = matrix4.transformPoint([this.originPoint[0], this.originPoint[1]]); // error
  private matrix2 = matrix4.identity().scale({ x: 2 });

  build() {
    Column() {
      Text('Test')
        .width("40%")
        .height(100)
        .transform(this.matrix1)
      Text(`矩阵变换后的坐标：[${this.transformPoint}]`)
        .fontSize(16)
        .margin({ top: 100 })
      Text("test")
        .transform(matrix4.combine(this.matrix2))// error
        .width("40%")
        .height(100)
        .margin({ top: 50 })
    }
  }
}