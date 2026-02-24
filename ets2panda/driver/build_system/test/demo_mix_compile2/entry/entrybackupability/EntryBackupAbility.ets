/*---
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
---*/
import { hilog } from '@kit.PerformanceAnalysisKit';
import { BackupExtensionAbility } from '@kit.CoreFileKit';

const DOMAIN = 0x0000;

export default class EntryBackupAbility extends BackupExtensionAbility {
    async onBackup() {
        hilog.info(DOMAIN, 'testTag', 'onBackup ok');
    }

    onCreate(want: Want, launchParam: AbilityConstant.LaunchParam): void {
        this.context.getApplicationContext().setColorMode(ConfigurationConstant.ColorMode.COLOR_MODE_NOT_SET);
        hilog.info(DOMAIN, 'testTag', '%{public}s', 'Ability onCreate');
    }

    onDestroy(): void {
        hilog.info(DOMAIN, 'testTag', '%{public}s', 'Ability onDestroy');
    }

    onWindowStageCreate(windowStage: window.WindowStage): void {
        hilog.info(DOMAIN, 'testTag', '%{public}s', 'Ability onWindowStageCreate');
        
        windowStage.loadContent('pages/Index', (err) => {
            if (err.code) {
                hilog.error(DOMAIN, 'testTag', 'Failed to load the content. Cause: %{public}s', JSON.stringify(err));
                return;
            }
            hilog.info(DOMAIN, 'testTag', 'Successeded in loading the content');
        });
    }

    onWindowStageDestroy(): void {
        hilog.info(DOMAIN, 'testTag', '%{public}s', 'Ability onWindowStageDestroy');
    }

    onForeground(): void {
        hilog.info(DOMAIN, 'testTag', '%{public}s', 'Ability onForeground');
    }

    onBackground(): void {
        hilog.info(DOMAIN, 'testTag', '%{public}s', 'Ability onBackground');
    }
}