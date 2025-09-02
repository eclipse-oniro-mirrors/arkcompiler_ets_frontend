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
} from '@kit.ArkUI';

import { A } from './har'
import { common2D } from '@kit.ArkGraphics2D'
import Base from '@ohos.base'
import { window } from '@kit.ArkUI'

A.getContext()

A.b.getContext()

A.getInstance().getContext()

let a: A = new A()
a.c.getContext()

let c = a.c
c.getContext()

const err = (err: Base.BusinessError) => {}

export const DEFAULT_WINDOW_SIZE: window.size = { width: 1280, height: 2580}

function test (aa: common2D.Rect) {

}

@Entry
@Component
struct importTest {
  build() {
  }
}