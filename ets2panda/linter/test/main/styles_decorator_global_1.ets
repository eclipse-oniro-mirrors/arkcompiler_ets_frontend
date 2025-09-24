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

const mycolor: string = "#ffff00"

@Styles
function cardStyle1()
{
    .backgroundColor(mycolor)
    .borderColor(Color.Red)
    .borderRadius(8)
    .padding(8)
    .backgroundImagePosition({
        x: 0,
        y: 0
    });
}

@Component
struct MyCard1 {
    build() {
        Column() {
            Text("TestStyles").cardStyle1()
        }
        .cardStyle1()
        .backgroundColor(Color.Red)
    }
}

@Styles
function NormalStyles() {
    .backgroundColor("#ffffff")
}

@Styles
function PressedStyles() {
    .backgroundColor("#ffffff")
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

@Styles
function StyleWithComment() {
    // dummy comment
    .backgroundColor("#ffffff")
}

@Styles
function StyleWithComment2() {
    // dummy comment
    .backgroundColor("#ffffff")
    // another comment
    .borderColor(Color.Red)
    // one more really important comment
    .borderRadius(8)
    .padding(8)
}
