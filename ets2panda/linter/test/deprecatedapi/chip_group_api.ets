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
import {ChipGroupItemOptions} from './sdk/api/ChipGroup';

@Entry
struct Index {
  build() {
    Column() {
      ChipGroup({
        items: [
          {
            prefixIcon: { src: $r('app.media.icon') },
            label: { text: "操作块1" },
            .suffixIcon: { src: $r('sys.media.ohos_ic_public_cut') }, //error
            allowClose: false
          }
        ]
      })
    }
  }
}
class Test implements ChipGroupItemOptions{
  suffixIcon?: IconOptions | undefined;
  get(){
    return this.suffixIcon;
  }
  set(suffixIcon?: IconOptions | undefined){
    suffixIcon = { src: $r('sys.media.ohos_ic_public_cut') };
  }
}