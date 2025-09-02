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
import {DragEvent,CustomPopupOptions} from './sdk/api/common';
import {LayoutInfo,LayoutChild,LayoutBorderInfo,TransitionOptions} from './sdk/api/common';

function get(aa:number){
  const globalEvent : DragEvent = {}
  return globalEvent.getX();//error
}
function onMeasure(selfLayoutInfo: LayoutInfo, borderInfo: Array<LayoutBorderInfo>, children: Array<LayoutChild>){//error
  selfLayoutInfo as LayoutInfo;//error
  typeof (borderInfo[0] as LayoutBorderInfo);//error
}

class Test implements LayoutChild{//error
  set(child:LayoutChild){//error
    return child as LayoutInfo;//error
  }
}
class Test2 extends  Test implements LayoutBorderInfo,TransitionOptions{ //error
  private layoutInfo: LayoutInfo|undefined = undefined;//error
  get(option:Map<string,TransitionOptions>){//error
    option.set('',{});
  }
}

@Entry
@Component
struct Example {
  @State uri: string = "";
  @State blockArr: string[] = [];
  @State handlePopup: boolean = false;
  @State customPopup: boolean = false;
  udKey: string = '';
  globalEvent : DragEvent = {}

  @State handlePopup: boolean = false
  private scroller: Scroller = new Scroller();

  aboutToAppear(): void {
    animateTo({ // error
      duration: 2000,
      curve: Curve.EaseOut,
      iterations: 3,
      playMode: PlayMode.Normal,
      onFinish: () => {
        console.info('play end');
      }
    }, () => {
    });
  }

  onLayout(children: LayoutChild[], constraint: ConstraintSizeOptions): void {// error *2
    let layoutChild: LayoutChild = children[0];// error
    layoutChild.constraint; // error
    layoutChild.position; // error
    layoutChild.name; // error
    layoutChild.id; // error
    layoutChild.borderInfo.borderWidth; // error *2
    layoutChild.borderInfo.padding; // error *2
    layoutChild.borderInfo.margin; // error *2
    layoutChild.layout({ position: { x: 0, y: 0 }, constraint: constraint }); // error *3
    layoutChild.measure(constraint); // error
  }

  onMeasure(children: LayoutChild[], constraint: ConstraintSizeOptions): void {// error *2
  }

  // Popup builder
  @Builder popupBuilder() {
    Row({ space: 2 }) {
    }.width(100).height(50).padding(5)
  }
  build() {
    Column() {
      Text('Image drag and drop')
        .fontSize('30dp')
      Flex({ direction: FlexDirection.Row, alignItems: ItemAlign.Center, justifyContent: FlexAlign.SpaceAround }) {
        Image($r('app.media.startIcon'))
          .width(100)
          .height(100)
          .border({ width: 1 })
          .draggable(true)
          .onDragStart((event:DragEvent) => {
            typeof event.getX(); //error
            (event as DragEvent).getY(); //error
            console.log('',event.getY()); //error
          })
      }
      .margin({ bottom: this.globalEvent.getY() }) //error
      Row() {
        Column(){
        }
        .border({width: 1})
        .height('90%')
        .width('100%')
        .onDrop((event?: DragEvent, extraParams?: string) => {
          console.log("enter onDrop")
          const x = event?.getX(); //error
          const y = event?.getY(); //error
        }, {disableDataPrefetch: true})
      }
      .height("50%")
      .width("90%")
      .border({ width: 1 })

      Button('PopupOptions')
        .bindPopup(this.handlePopup, {
          message: 'This is a popup with PopupOptions',
          placementOnTop: true// error
        })
        .useSizeType({}) // error

      List({ space: 10, scroller: this.scroller })
      .onScroll((xOffset: number, scrollState: ScrollState): void => { // error
      })
      .width('100%')
      .height(90)
      .touchable(true)// error
      .mask(new CircleAttribute()) // error
    }
  }
  onLayout(children: Array<LayoutChild>//error
    , constraint: ConstraintSizeOptions) {
    children[0].borderInfo as LayoutBorderInfo;//error
  }
}