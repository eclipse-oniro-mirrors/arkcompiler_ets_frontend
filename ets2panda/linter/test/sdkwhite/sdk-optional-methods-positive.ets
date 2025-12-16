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
  updateRect(x: number, y: number, width: number, height: number): void {
    throw new Error('Method not implemented.');
  }

  play(): void {
    throw new Error('Method not implemented.');
  }

  pause(): void {
    throw new Error('Method not implemented.');
  }

  seek(targetTime: number): void {
    throw new Error('Method not implemented.');
  }

  setVolume(volume: number): void {
    throw new Error('Method not implemented.');
  }

  setMuted(muted: boolean): void {
    throw new Error('Method not implemented.');
  }

  setPlaybackRate(playbackRate: number): void {
    throw new Error('Method not implemented.');
  }

  release(): void {
    throw new Error('Method not implemented.');
  }

  enterFullscreen(): void {
    throw new Error('Method not implemented.');
  }

  exitFullscreen(): void {
    throw new Error('Method not implemented.');
  }
}

class MyAbilityLifecycleCallback extends AbilityLifecycleCallback {
  onWillNewWant(ability: UIAbility) {
    console.log('AbilityLifecycleCallback onWillNewWant.');
  }

  onAbilityWillForeground: (ability: UIAbility) => void =
    (ability: UIAbility) => {
      console.log('AbilityLifecycleCallback onAbilityWillForeground');
    }
}

const callback: AbilityLifecycleCallback = {
  onAbilityWillForeground: (ability: UIAbility): void => {
      console.log('AbilityLifecycleCallback onAbilityWillForeground');
    }
} as AbilityLifecycleCallback;

class GenericHandler<T extends UIAbility> extends AbilityLifecycleCallback {
  onAbilityWillForeground: (ability: T) => void =
    (ability: T) => {
      console.log('AbilityLifecycleCallback onAbilityWillForeground');
    }
}
