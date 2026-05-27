/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
'use static'

import { StringUtils } from 'har';

export class NonPageUtils {
  static getUtilsData(): string {
    return "文件名会被混淆";
  }
}

class A extends NonPageUtils {
  private util:StringUtils=new StringUtils();
  static getData(): string {
    return "";
  }
}
export function getMyRandomStr():void {
  StringUtils.getRandomStr(8);
}

export function hspFunction(): void {
  console.log('HSP function called');
}