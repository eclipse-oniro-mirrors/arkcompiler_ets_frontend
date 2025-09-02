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
import {NavigatorInstance,Navigator,NavigatorAttribute} from './sdk/api/navigator';

function aa(tt:NavigatorAttribute){//error
  const  a = new NavigatorAttribute() //error
  typeof a.active(false);//error
  return a as NavigatorAttribute;//error
}
@Entry
@Component
struct NavigatorExample {
  @State active: boolean = false;
  @State name: NameObject = { name: 'news' };

  build() {
    Flex({
      direction: FlexDirection.Column,
      alignItems: ItemAlign.Start,
      justifyContent: FlexAlign.SpaceBetween
    }) {
      Navigator({//error 
        target: 'pages/container/navigator/Detail',
        type: NavigationType.Push
      }).onClick(()=>{
        const attr = new NavigatorAttribute(); //error 
        attr.target('111')//error
      }).params(new TextObject(this.name))
      NavigatorInstance.active(false);//error
    }.height(150).width(350).padding(35);
  }
}