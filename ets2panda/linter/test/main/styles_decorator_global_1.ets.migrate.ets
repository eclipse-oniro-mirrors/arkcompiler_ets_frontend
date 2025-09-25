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

import { applyStyles } from '@kit.ArkUI';

import {
  CommonMethod,
  Color,
  Component,
  Column,
  Text,
  BuilderParam,
  Require,
  Button,
} from '@kit.ArkUI';

const mycolor: string = "#ffff00"

function cardStyle1(instance: CommonMethod): void
{
    instance.backgroundColor(mycolor);
    instance.borderColor(Color.Red);
    instance.borderRadius(8);
    instance.padding(8);
    instance.backgroundImagePosition({
        x: 0,
        y: 0
    });
}

@Component
struct MyCard1 {
    build() {
        Column() {
            Text("TestStyles").applyStyles(cardStyle1)
        }
        .applyStyles(cardStyle1)
        .backgroundColor(Color.Red)
    }
}

function NormalStyles(instance: CommonMethod): void {
    instance.backgroundColor("#ffffff");
}

function PressedStyles(instance: CommonMethod): void {
    instance.backgroundColor("#ffffff");
}

@Component
struct MyButton {
    @BuilderParam
    @Require
    content: () => void;

    build() {
        Button() {
            this.content()
        }
        .stateStyles({
            normal: NormalStyles,
            pressed: PressedStyles
        })
    }
}

function StyleWithComment(instance: CommonMethod): void {
    // dummy comment
    instance.backgroundColor("#ffffff");
}

function StyleWithComment2(instance: CommonMethod): void {
    // dummy comment
    instance.backgroundColor("#ffffff");
    // another comment
    instance.borderColor(Color.Red);
    // one more really important comment
    instance.borderRadius(8);
    instance.padding(8);
}
