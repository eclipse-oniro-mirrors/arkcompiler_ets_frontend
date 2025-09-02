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
  CustomStyles,
  applyStyles,
} from '@kit.ArkUI';

import {
  Component,
  CommonMethod,
  Column,
  Text,
  Color,
  BuilderParam,
  Require,
  Button,
  CustomBuilder,
} from '@kit.ArkUI';

@Component
struct MyCard1 {
  cardStyle1: CustomStyles = (instance: CommonMethod): void => {
    instance.backgroundColor('#ffffff');
    instance.borderRadius(8);
  };

  build() {
    Column() {
      Text('Card').applyStyles(this.cardStyle1)
    }
    .applyStyles(this.cardStyle1)
    .backgroundColor(Color.Red)
  }
}

@Component
struct MyCard2 {
  cardStyle2: CustomStyles = (instance: CommonMethod): void => {
    instance.border({
        color: this.getColor(),
        width: this.getWidth()
    });
    instance.backgroundColor('#ffffff');
  };
  
  private getColor(): Color {
    return Color.Red
  }
  
  private getWidth(): number {
    return 10
  }

  build() {
    Column() {
      Text('Card').applyStyles(this.cardStyle2)
    }
    .applyStyles(this.cardStyle2)
    .backgroundColor(Color.Red)
  }
}

@Component
struct MyButton {
  @BuilderParam
  @Require
  content: () => void;

  NormalStyles: CustomStyles = (instance: CommonMethod): void => {
    instance.backgroundColor(Color.Red);
  };

  PressedStyles: CustomStyles = (instance: CommonMethod): void => {
    instance.backgroundColor(Color.Green);
  };

  build() {
    Button() {
      this.content
    }
    .stateStyles({
      normal: this.NormalStyles,
      pressed: this.PressedStyles
    })
  }
}

const TAG: string = 'common_ImageTest';

class MediaItem {
  public path: string = '';
  public uri: string = '';

  public getDateModified(): string {
    return ''
  }
}

@Component
struct imageTest {
  private mediaItem: MediaItem | undefined;
  private previewUri: string | undefined;
  private DragBuilder: CustomBuilder | undefined;

  imageStyle: CustomStyles = (instance: CommonMethod): void => {
    instance.draggable(this.isShowLongPressMenu() && this.isPC());
    instance.onDragStart(() => {
        console.info(TAG, 'onDragStart');
        this.touchVibrate(VibrateType.DRAG);
        if (this.mediaItem?.path) {
            this.previewUri =
                this.getPreviewUri(this.mediaItem?.path, this.mediaItem?.getDateModified?.(), false, true);
        }
        return this.DragBuilder;
    });
    instance.accessibilityText(this.isOpenTouchGuide ? this.getImageItemGridAccessibilityText() : '');
  };

  private isShowLongPressMenu(): boolean {
    return true;
  }

  private isPC(): boolean {
    return true;
  }

  private touchVibrate(type: VibrateType): void {

  }

  private isOpenTouchGuide: boolean = true;

  private getImageItemGridAccessibilityText(): string {
    return ''
  }

  private getPreviewUri(path: string, date: string, isOk: boolean, ready: boolean): string {
    return ''
  }

  build() {

  }
}

enum VibrateType {
  DRAG
}