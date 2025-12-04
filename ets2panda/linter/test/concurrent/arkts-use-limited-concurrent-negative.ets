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

"use concurrent"      // It is the very first element of the file, so it is a directive

function concurrentFunc() {
    "use concurrent"    // It is used at the start of the function, so it is a directive
}

function refreshToGetFontScale(): number {
  'use concurrent';       // It is the first element, so it is a directive
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