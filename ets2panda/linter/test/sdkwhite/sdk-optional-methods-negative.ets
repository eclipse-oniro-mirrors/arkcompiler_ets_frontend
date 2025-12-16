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

import { webview } from '@kit.ArkWeb';
import { AbilityLifecycleCallback, UIAbility } from '@kit.AbilityKit';

class OptionalMethod implements webview.NativeMediaPlayerBridge {
  resumePlayer?(): void { // Error incorrect sdk method
    throw new Error('Method not implemented.');
  }

  suspendPlayer?(type: webview.SuspendType): void { // Error incorrect sdk method
    throw new Error('Method not implemented.');
  }
}

class MyAbilityLifecycleCallback extends AbilityLifecycleCallback {
  onAbilityWillCreate?(ability: UIAbility) { // error incorrect optional sdk method
    console.log('AbilityLifecycleCallback onAbilityWillCreate.');
  }

  onAbilityWillBackground?: (ability: UIAbility) => void = // error optional sdk method
    (ability: UIAbility) => {
      console.log('AbilityLifecycleCallback onAbilityWillBackground');
  }
}

class GenericHandler<T extends UIAbility> extends AbilityLifecycleCallback {
  onAbilityWillCreate(ability: T): void { /*...*/ } // error incorrect optional sdk method
}
