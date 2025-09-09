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

import { UIAbility } from '@kit.AbilityKit';
import { UIExtensionAbility } from '@kit.AbilityKit';
import { AutoFillExtensionAbility } from '@kit.AbilityKit';
import { ServiceExtensionAbility } from '@kit.AbilityKit';

function sleep(ms: number): Promise<void> {
  return new Promise((resolve, reject) => {
    setTimeout(resolve, ms)
  })
}

function sleep1(ms: number): void {
  return void
}

export default class MyUIAbility1 extends UIAbility {
  async onDestroy(): Promise<void> { // use UIAbility onDestroy, should report error
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onDestroy');
    return sleep(1000);
  }
}

export default class MyUIAbility2 extends UIExtensionAbility {
  async onDestroy(): Promise<void> { // use UIExtensionAbility onDestroy, should report error
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onDestroy');
    return sleep(1000);
  }
}

export default class MyUIAbility3 extends AutoFillExtensionAbility {
  async onDestroy(): Promise<void> { // use AutoFillExtensionAbility onDestroy, should report error
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onDestroy');
    return sleep(1000);
  }
}

export default class MyUIAbility4 extends ServiceExtensionAbility {
  async onDisconnect(): Promise<void> { // use UIAbility onDestroy, should report error
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onDestroy');
    return sleep(1000);
  }
}

export default class MyUIAbility5 extends UIAbility {
  onDestroy() { // use UIAbility onDestroy with Promise<void> return, should report error
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onDestroy');
    return sleep(1000);
  }
}

export default class MyUIAbility6 extends UIAbility {
  onDestroy() { // use UIAbility onDestroy with void return, should not report error
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onDestroy');
    return sleep1(1000);
  }
}

export default class MyUIAbility7 extends MyUIAbility1 {
  async onDestroy(): Promise<void> { // use UIAbility onDestroy, should report error
    hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onDestroy');
    return sleep(1000);
  }
}
