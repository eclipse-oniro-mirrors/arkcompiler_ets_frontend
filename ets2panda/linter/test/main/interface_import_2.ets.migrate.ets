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

import { Entry, Component, State, Column, Button, Text, Row, TextAttribute, ColumnAttribute, AnimatableExtend, $$, ImageFit } from '@kit.ArkUI';

import { Slider } from '@kit.ArkUI';

@Entry
@Component
struct Test {
  @State num: number = 0
  @State a: MyClassA = new MyClassA()

  build() {
    Column() {
      Button('按钮')
        .backgroundColor('#ffffff')
      Circle() {
        
      }

      MyComponent1() {

      }

      Text("Test")
    }.width("100%")
  }
}

enum Color {
  Green
}

function Circle() {

}

@Component
struct MyComponent1 {
  @State count: number = 0;

  build() {
    Row() {
      Slider(){}
    }
  }
}

function cardStyle(this: TextAttribute): this {
    this.backgroundColor(Color.Green);
    return this;
}

@AnimatableExtend
function animatableWidth(this: ColumnAttribute, width: number): this {
    this.width(width);
    return this;
}

@Component
struct MyComponent2 {
  @State value: number = 0;

  build() {
    Row() {
      Slider({
        value: $$(this.value)
      })
    }
  }
}

class MyClassA {}

function processImageFit(imageFit: ImageFit): void {
}

function Calendar() {

}