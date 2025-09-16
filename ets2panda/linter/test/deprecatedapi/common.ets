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
 
@Entry
@Component
struct ClipExample {
  build() {
    Column({ space: 15 }) {
      Row() {
        Image($r('app.media.testImg'))
          .clip(new Circle({})) //error

        Button()
          .transition({
            type: TransitionType.Insert, //error
            opacity: 0, //error
            translate: {x: 1}, //error
            scale: {x: 1}, //error
            rotate: {angle: 1} //error
          })
          .onClick(() => {
            let context: Context = getContext(this); //error
            console.info("CacheDir:" + context.cacheDir);
          })
		
		Button('Touch').height(40).width(100)
        .onTouch((event?: TouchEvent) => {
          if (event) {
            event.touches[0].screenX; //error
            event.touches[0].screenY; //error
          }
        })
        .onClick((event?: ClickEvent) => {
          if (event) {
            event.screenX; //error
            event.screenY; //error
          }
        })
        .onMouse((event: MouseEvent):void => {
          if (event) {
            event.screenX; //error
            event.screenY; //error
          }
        })
      }
      .borderRadius(20)
    }
    .width('100%')
    .margin({ top: 15 })
  }
}


px2lpx(200); //error
lpx2px(200); //error
px2fp(200); //error
fp2px(200); //error
px2vp(200); //error
vp2px(200); //error