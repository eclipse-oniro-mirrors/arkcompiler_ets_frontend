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
import {IndicatorStyle,SwiperDisplayMode} from './sdk/api/swiper';

class Test extends  Test implements IndicatorStyle{ //error
  layoutInfo: IndicatorStyle|undefined = undefined;//error
  get(option:Map<string,IndicatorStyle>){//error
    option.set('',{top:100,color:1111});//error
    option.set('',{bottom:SwiperDisplayMode.Stretch})//error
  }
  getInfo(){
    return this.layoutInfo;
  }
  set(){
    typeof this.layoutInfo?.left//error
    this.layoutInfo?.right == 100//error
    this.layoutInfo?.color?.toString()//error
    const info = this.layoutInfo;
    console.log(info?.selectedColor+'');//error
    this.getInfo()?.mask//error
    this.layoutInfo?.bottom//error
    setTop(this.getInfo())
  }
  setSize(layoutInfo: IndicatorStyle|undefined){//error
    layoutInfo?.size ;//error
  }
}
function setTop(layoutInfo: IndicatorStyle|undefined ){//error
  layoutInfo?.top == 0;//error
}
function getBotton(){
  new Test().getInfo()?.bottom;//error
  const test = new Test();
  return test.layoutInfo?.bottom;//error
}
function get(){
  const AutoLinear = SwiperDisplayMode.AutoLinear//error
  typeof SwiperDisplayMode.Stretch;//error
  return SwiperDisplayMode.AUTO_LINEAR;//error
}