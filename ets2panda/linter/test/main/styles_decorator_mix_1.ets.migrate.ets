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
  CommonMethod,
  Color,
  Entry,
  Component,
  BuilderParam,
  Require,
  Button,
  CustomStyles,
} from '@kit.ArkUI';

function NormalStyles(instance: CommonMethod): void {
    instance.backgroundColor(Color.Blue)
}

@Entry
@Component
struct MyButton1 {
  @BuilderParam
  @Require
  content: () => void;

  PressedStyles: CustomStyles = (instance: CommonMethod): void => {
    instance.backgroundColor(Color.Green)
  };

  build() {
    Button("MyButton1") {
      this.content
    }
    .stateStyles({
      pressed: this.PressedStyles,
      selected: (instance: CommonMethod): void => {
          instance.backgroundColor(Color.Red);
      },
      normal: NormalStyles
    })
  }
}

@Component
struct MyButton2 {
  @BuilderParam
  @Require
  content: () => void;

  build() {
    Button("MyButton2") {
      this.content
    }
    .stateStyles({
      selected: (instance: CommonMethod): void => {
          instance.backgroundColor(Color.Red);
      },
      normal: NormalStyles
    })
  }
}

@Component
struct MyButton3 {

  build() {
    Button("MyButton3")
      .stateStyles({
        normal: NormalStyles,
        selected: (instance: CommonMethod): void => {
            instance.backgroundColor(Color.Red);
        }
      })
  }
}

@Component
struct MyButton4 {

  build() {
    Button("MyButton4")
      .stateStyles({
        selected: (instance: CommonMethod): void => {
            instance.backgroundColor(Color.Red);
        },
        normal: NormalStyles
      })
  }
}