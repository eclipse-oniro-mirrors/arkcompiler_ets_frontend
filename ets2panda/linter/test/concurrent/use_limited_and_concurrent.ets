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

"use shared"
export function sharedFunction() {}

function concurrentFunc() {
    "use concurrent"
}

function normal() {}

normal('use concurrent');

const variable = 'use concurrent';

switch (variable) {
    case 'use concurrent:
        break;
}

normal('use shared');

const variable2 = 'use shared';

switch (variable2) {
    case 'use shared':
        break;
}

function refreshToGetFontScale(): number {
  'use concurrent';
  const TAG = 'SCBScreenLock-refreshToGetFontScale';
  const log: LogHelper = LogHelper.getLogHelper(LogDomain.KG, TAG);
  const FONT_SCALE_DEFAULT: number = 1;
  try {
    return uiAppearance.getFontScale();
  } catch (err) {
    log.showError(`uiAppearance getFontScale err,code:${err?.code}`);
  }
  return FONT_SCALE_DEFAULT;
}
